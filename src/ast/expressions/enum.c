#include <assert.h>

#include "ast/expressions/enum.h"
#include "ast/expressions/identifier.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_enum_variant_list(ArrayList* variants, free_alloc_fn free_alloc) {
    assert(variants);
    assert(free_alloc);

    EnumVariant variant;
    for (size_t i = 0; i < variants->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(variants, i, &variant));
        identifier_expression_destroy((Node*)variant.name, free_alloc);
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
    ASSERT_NODE(node);
    assert(free_alloc);

    EnumExpression* e = (EnumExpression*)node;
    free_enum_variant_list(&e->variants, free_alloc);

    free_alloc(e);
}

NODISCARD Status enum_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    EnumExpression* e = (EnumExpression*)node;
    TRY(string_builder_append_str_z(sb, "enum { "));

    EnumVariant variant;
    for (size_t i = 0; i < e->variants.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&e->variants, i, &variant));

        TRY(identifier_expression_reconstruct((Node*)variant.name, symbol_map, sb));
        if (variant.value) {
            Node* value_node = (Node*)variant.value;
            TRY(string_builder_append_str_z(sb, " = "));
            TRY(value_node->vtable->reconstruct(value_node, symbol_map, sb));
        }

        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append(sb, '}'));
    return SUCCESS;
}

NODISCARD Status enum_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
