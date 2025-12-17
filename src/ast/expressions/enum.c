#include <assert.h>
#include <stdalign.h>

#include "ast/expressions/enum.h"
#include "ast/expressions/identifier.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/hash_set.h"
#include "util/containers/string_builder.h"

void free_enum_variant_list(ArrayList* variants, free_alloc_fn free_alloc) {
    assert(variants);
    assert(free_alloc);

    ArrayListIterator it = array_list_iterator_init(variants);
    EnumVariant       variant;
    while (array_list_iterator_has_next(&it, &variant)) {
        NODE_VIRTUAL_FREE(variant.name, free_alloc);
        NODE_VIRTUAL_FREE(variant.value, free_alloc);
    }

    array_list_deinit(variants);
}

NODISCARD Status enum_expression_create(Token            start_token,
                                        ArrayList        variants,
                                        EnumExpression** enum_expr,
                                        memory_alloc_fn  memory_alloc) {
    assert(variants.item_size == sizeof(EnumVariant));
    assert(variants.length > 0);
    assert(memory_alloc);

    EnumExpression* enum_local = memory_alloc(sizeof(EnumExpression));
    if (!enum_local) {
        return ALLOCATION_FAILED;
    }

    *enum_local = (EnumExpression){
        .base     = EXPRESSION_INIT(ENUM_VTABLE, start_token),
        .variants = variants,
    };

    *enum_expr = enum_local;
    return SUCCESS;
}

void enum_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    EnumExpression* enum_expr = (EnumExpression*)node;
    free_enum_variant_list(&enum_expr->variants, free_alloc);

    free_alloc(enum_expr);
}

NODISCARD Status enum_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    EnumExpression* enum_expr = (EnumExpression*)node;
    TRY(string_builder_append_str_z(sb, "enum { "));

    ArrayListIterator it = array_list_iterator_init(&enum_expr->variants);
    EnumVariant       variant;
    while (array_list_iterator_has_next(&it, &variant)) {
        ASSERT_EXPRESSION(variant.name);
        TRY(NODE_VIRTUAL_RECONSTRUCT(variant.name, symbol_map, sb));

        if (variant.value) {
            ASSERT_EXPRESSION(variant.value);
            TRY(string_builder_append_str_z(sb, " = "));
            TRY(NODE_VIRTUAL_RECONSTRUCT(variant.value, symbol_map, sb));
        }

        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append(sb, '}'));
    return SUCCESS;
}

NODISCARD Status enum_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token     start_token    = node->start_token;
    const Allocator allocator      = parent->symbol_table->symbols.allocator;
    const Slice     enum_type_name = parent->namespace_type_name;

    EnumExpression* enum_expr = (EnumExpression*)node;
    assert(enum_expr->variants.data && enum_expr->variants.length > 0);
    parent->namespace_type_name = zeroed_slice();

    SemanticType* type;
    TRY(semantic_type_create(&type, allocator.memory_alloc));
    type->tag      = STYPE_ENUM;
    type->is_const = true;
    type->nullable = false;
    type->valued   = false;

    HashSet variants;
    TRY_DO(hash_set_init_allocator(&variants,
                                   4,
                                   sizeof(MutSlice),
                                   alignof(MutSlice),
                                   hash_mut_slice,
                                   compare_mut_slices,
                                   allocator),
           rc_release(type, allocator.free_alloc););

    ArrayListConstIterator it = array_list_const_iterator_init(&enum_expr->variants);
    EnumVariant            variant;
    while (array_list_const_iterator_has_next(&it, &variant)) {
        IdentifierExpression* ident = (IdentifierExpression*)variant.name;
        char*                 duped =
            strdup_s_allocator(ident->name.ptr, ident->name.length, allocator.memory_alloc);
        if (!duped) {
            rc_release(type, allocator.free_alloc);
            free_enum_variant_set(&variants, allocator.free_alloc);
            return ALLOCATION_FAILED;
        }
        const MutSlice variant_name = mut_slice_from_str_s(duped, ident->name.length);

        // Shadowing of variant names is not allowed inside or outside the parent context
        if (symbol_table_has(parent->symbol_table, variant_name) ||
            hash_set_contains(&variants, &variant_name) ||
            mut_slice_equals_slice(&variant_name, &enum_type_name)) {
            Status error_code = REDEFINITION_OF_IDENTIFIER;
            if (mut_slice_equals_slice(&variant_name, &enum_type_name)) {
                error_code = NAMESPACE_NAME_MIRRORS_MEMBER;
            }

            IGNORE_STATUS(
                put_status_error(errors, error_code, start_token.line, start_token.column));

            rc_release(type, allocator.free_alloc);
            free_enum_variant_set(&variants, allocator.free_alloc);
            allocator.free_alloc(duped);
            return error_code;
        }

        // The value type of a variant is ephemeral and optional
        if (variant.value) {
            TRY_DO(NODE_VIRTUAL_ANALYZE(variant.value, parent, errors), {
                rc_release(type, allocator.free_alloc);
                free_enum_variant_set(&variants, allocator.free_alloc);
                allocator.free_alloc(duped);
            });

            // Explicit values must be constant, non-nullable, signed integers
            SemanticType* variant_type      = semantic_context_move_analyzed(parent);
            Status        type_check_result = SUCCESS;
            if (variant_type->nullable) {
                type_check_result = NULLABLE_ENUM_VARIANT;
            } else if (!variant_type->is_const) {
                type_check_result = NON_CONST_ENUM_VARIANT;
            } else if (variant_type->tag != STYPE_SIGNED_INTEGER) {
                type_check_result = NON_SIGNED_ENUM_VARIANT;
            } else if (!variant_type->valued) {
                type_check_result = NON_VALUED_ENUM_VARIANT;
            }

            // Since the types are ephemeral we just release now before error code is sent
            rc_release(variant_type, allocator.free_alloc);
            if (STATUS_ERR(type_check_result)) {
                IGNORE_STATUS(put_status_error(
                    errors, type_check_result, start_token.line, start_token.column));

                rc_release(type, allocator.free_alloc);
                free_enum_variant_set(&variants, allocator.free_alloc);
                allocator.free_alloc(duped);
                return type_check_result;
            }
        }

        // Now the variant must be valid and should be added to the set
        TRY_DO(hash_set_put(&variants, &variant_name), {
            rc_release(type, allocator.free_alloc);
            free_enum_variant_set(&variants, allocator.free_alloc);
            allocator.free_alloc(duped);
        });
    }

    // The enum stores a reference to its parent name
    SemanticEnumType* enum_type;
    TRY_DO(semantic_enum_create(enum_type_name, variants, &enum_type, allocator.memory_alloc), {
        rc_release(type, allocator.free_alloc);
        free_enum_variant_set(&variants, allocator.free_alloc);
    });

    type->variant.enum_type = enum_type;
    parent->analyzed_type   = type;
    return SUCCESS;
}
