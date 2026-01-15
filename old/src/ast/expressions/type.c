#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/enum.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status type_expression_create(Token               start_token,
                                            TypeExpressionTag   tag,
                                            TypeExpressionUnion variant,
                                            TypeExpression**    type_expr,
                                            Allocator*          allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    TypeExpression* type = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*type));
    if (!type) { return ALLOCATION_FAILED; }

    *type = (TypeExpression){
        .base    = EXPRESSION_INIT(TYPE_VTABLE, start_token),
        .tag     = tag,
        .variant = variant,
    };

    *type_expr = type;
    return SUCCESS;
}

void type_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);
    TypeExpression* type = (TypeExpression*)node;

    if (type->tag == EXPLICIT) {
        ExplicitType explicit_type = type->variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT:
            ExplicitIdentType ident_type = explicit_type.variant.ident_type;
            NODE_VIRTUAL_FREE(ident_type.name, allocator);
            free_expression_list(&ident_type.generics, allocator);
            break;
        case EXPLICIT_TYPEOF:
            NODE_VIRTUAL_FREE(explicit_type.variant.referred_type, allocator);
            break;
        case EXPLICIT_FN:
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            free_expression_list(&function_type.generics, allocator);
            free_parameter_list(&function_type.parameters, allocator);
            NODE_VIRTUAL_FREE(function_type.return_type, allocator);
            break;
        case EXPLICIT_STRUCT:
            NODE_VIRTUAL_FREE(explicit_type.variant.struct_type, allocator);
            break;
        case EXPLICIT_ENUM:
            NODE_VIRTUAL_FREE(explicit_type.variant.enum_type, allocator);
            break;
        case EXPLICIT_ARRAY:
            TypeExpression* inner = explicit_type.variant.array_type.inner_type;
            array_list_deinit(&explicit_type.variant.array_type.dimensions);
            NODE_VIRTUAL_FREE(inner, allocator);
            break;
        }
    }

    ALLOCATOR_PTR_FREE(allocator, type);
}

[[nodiscard]] Status
type_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TypeExpression* type = (TypeExpression*)node;
    if (type->tag == EXPLICIT) {
        ExplicitType explicit_type = type->variant.explicit_type;
        if (explicit_type.nullable) { TRY(string_builder_append(sb, '?')); }

        TRY(explicit_type_reconstruct(explicit_type, symbol_map, sb));
    } else {
        TRY(string_builder_append_str_z(sb, " :"));
    }
    return SUCCESS;
}

[[nodiscard]] Status
type_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token start_token = node->start_token;
    Allocator*  allocator   = semantic_context_allocator(parent);

    TypeExpression* type = (TypeExpression*)node;
    SemanticType*   new_symbol_type;

    if (type->tag == IMPLICIT) {
        TRY(semantic_type_create(&new_symbol_type, allocator));
        new_symbol_type->tag      = STYPE_IMPLICIT_DECLARATION;
        new_symbol_type->variant  = SEMANTIC_DATALESS_TYPE;
        new_symbol_type->is_const = true;
        new_symbol_type->valued   = false;
        new_symbol_type->nullable = false;
    } else {
        const ExplicitType explicit_type = type->variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT:
            TRY(semantic_type_create(&new_symbol_type, allocator));
            new_symbol_type->is_const = true;
            new_symbol_type->valued   = false;

            // TODO: Handle type generics
            ExplicitIdentType ident_type      = explicit_type.variant.ident_type;
            MutSlice          probe_type_name = ident_type.name->name;
            SemanticType*     probe_symbol_type;

            SemanticTypeTag symbol_type_tag;
            if (semantic_name_to_primitive_type_tag(probe_type_name, &symbol_type_tag)) {
                new_symbol_type->tag      = symbol_type_tag;
                new_symbol_type->variant  = SEMANTIC_DATALESS_TYPE;
                new_symbol_type->nullable = explicit_type.nullable;
            } else if (semantic_context_find(parent, true, probe_type_name, &probe_symbol_type)) {
                // Double nullable doesn't make sense, guard before making the new type
                if (probe_symbol_type->nullable && explicit_type.nullable) {
                    PUT_STATUS_PROPAGATE(errors,
                                         DOUBLE_NULLABLE,
                                         start_token,
                                         RC_RELEASE(new_symbol_type, allocator));
                }

                // Underlying copy ignores nullable, so inherit from either source
                TRY_DO(semantic_type_copy_variant(new_symbol_type, probe_symbol_type, allocator),
                       RC_RELEASE(new_symbol_type, allocator));
                new_symbol_type->nullable = probe_symbol_type->nullable || explicit_type.nullable;
            } else {
                PUT_STATUS_PROPAGATE(errors,
                                     UNDECLARED_IDENTIFIER,
                                     start_token,
                                     RC_RELEASE(new_symbol_type, allocator));
            }
            break;
        case EXPLICIT_TYPEOF:
            TRY(semantic_type_create(&new_symbol_type, allocator));

            // We need to copy into since we may be changing nullable status
            const Expression* referred = explicit_type.variant.referred_type;
            TRY_DO(NODE_VIRTUAL_ANALYZE(referred, parent, errors),
                   RC_RELEASE(new_symbol_type, allocator));
            SemanticType* analyzed = semantic_context_move_analyzed(parent);

            TRY_DO(semantic_type_copy_variant(new_symbol_type, analyzed, allocator), {
                RC_RELEASE(analyzed, allocator);
                RC_RELEASE(new_symbol_type, allocator);
            });

            RC_RELEASE(analyzed, allocator);
            new_symbol_type->nullable = explicit_type.nullable;
            break;
        case EXPLICIT_ENUM:
            // The enum is brand new lifting and we can safely alter nullable
            const EnumExpression* enum_expr = explicit_type.variant.enum_type;
            TRY(NODE_VIRTUAL_ANALYZE(enum_expr, parent, errors));

            new_symbol_type           = semantic_context_move_analyzed(parent);
            new_symbol_type->nullable = explicit_type.nullable;
            break;
        case EXPLICIT_ARRAY:
            const ExplicitArrayType array_expr = explicit_type.variant.array_type;
            assert(array_expr.dimensions.length > 0);
            TRY(NODE_VIRTUAL_ANALYZE(array_expr.inner_type, parent, errors));
            SemanticType* inner_array_type = semantic_context_move_analyzed(parent);

            TRY_DO(semantic_type_create(&new_symbol_type, allocator),
                   RC_RELEASE(inner_array_type, allocator));
            new_symbol_type->nullable = explicit_type.nullable;

            SemanticArrayType* sarray;
            if (array_expr.dimensions.length == 1) {
                size_t length;
                UNREACHABLE_IF_ERROR(array_list_get(&array_expr.dimensions, 0, &length));
                TRY_DO(semantic_array_create(STYPE_ARRAY_SINGLE_DIM,
                                             (SemanticArrayUnion){.length = length},
                                             inner_array_type,
                                             &sarray,
                                             allocator),
                       {
                           RC_RELEASE(inner_array_type, allocator);
                           RC_RELEASE(new_symbol_type, allocator);
                       });
            } else {
                ArrayList dimensions;
                TRY_DO(array_list_init_allocator(
                           &dimensions, array_expr.dimensions.length, sizeof(size_t), allocator),
                       {
                           RC_RELEASE(inner_array_type, allocator);
                           RC_RELEASE(new_symbol_type, allocator);
                       });

                ArrayListConstIterator it = array_list_const_iterator_init(&array_expr.dimensions);
                size_t                 dim;
                while (array_list_const_iterator_has_next(&it, &dim)) {
                    array_list_push_assume_capacity(&dimensions, &dim);
                }

                TRY_DO(semantic_array_create(STYPE_ARRAY_MULTI_DIM,
                                             (SemanticArrayUnion){.dimensions = dimensions},
                                             inner_array_type,
                                             &sarray,
                                             allocator),
                       {
                           array_list_deinit(&dimensions);
                           RC_RELEASE(inner_array_type, allocator);
                           RC_RELEASE(new_symbol_type, allocator);
                       });
            }

            new_symbol_type->tag     = STYPE_ARRAY;
            new_symbol_type->variant = (SemanticTypeUnion){.array_type = sarray};
            break;
        default:
            return NOT_IMPLEMENTED;
        }

        assert(new_symbol_type->is_const);
        assert(!new_symbol_type->valued);
    }

    parent->analyzed_type = new_symbol_type;
    return SUCCESS;
}

[[nodiscard]] Status explicit_type_reconstruct(ExplicitType   explicit_type,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    assert(sb);

    switch (explicit_type.tag) {
    case EXPLICIT_IDENT:
        ExplicitIdentType ident_type = explicit_type.variant.ident_type;
        ASSERT_EXPRESSION(ident_type.name);
        TRY(string_builder_append_mut_slice(sb, ident_type.name->name));
        TRY(generics_reconstruct(&ident_type.generics, symbol_map, sb));
        break;
    case EXPLICIT_TYPEOF:
        ASSERT_EXPRESSION(explicit_type.variant.referred_type);
        TRY(string_builder_append_str_z(sb, "typeof "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.referred_type, symbol_map, sb));
        break;
    case EXPLICIT_FN:
        ExplicitFunctionType function_type = explicit_type.variant.function_type;
        TRY(string_builder_append_str_z(sb, "fn"));
        TRY(generics_reconstruct(&function_type.generics, symbol_map, sb));

        TRY(string_builder_append(sb, '('));
        TRY(reconstruct_parameter_list(&function_type.parameters, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, "): "));
        ASSERT_EXPRESSION(function_type.return_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(function_type.return_type, symbol_map, sb));
        break;
    case EXPLICIT_STRUCT:
        ASSERT_EXPRESSION(explicit_type.variant.struct_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.struct_type, symbol_map, sb));
        break;
    case EXPLICIT_ENUM:
        ASSERT_EXPRESSION(explicit_type.variant.enum_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.enum_type, symbol_map, sb));
        break;
    case EXPLICIT_ARRAY:
        assert(explicit_type.variant.array_type.dimensions.length > 0);
        TRY(string_builder_append(sb, '['));

        ArrayListConstIterator it =
            array_list_const_iterator_init(&explicit_type.variant.array_type.dimensions);
        size_t dim;
        while (array_list_const_iterator_has_next(&it, &dim)) {
            TRY(string_builder_append_unsigned(sb, (uint64_t)dim));
            TRY(string_builder_append_str_z(sb, "uz"));

            if (!array_list_const_iterator_exhausted(&it)) {
                TRY(string_builder_append_str_z(sb, ", "));
            }
        }

        TRY(string_builder_append(sb, ']'));

        TypeExpression* inner_type = explicit_type.variant.array_type.inner_type;
        assert(inner_type->tag == EXPLICIT);

        ExplicitType inner_explicit_type = inner_type->variant.explicit_type;
        if (inner_explicit_type.nullable) { TRY(string_builder_append(sb, '?')); }

        TRY(explicit_type_reconstruct(inner_explicit_type, symbol_map, sb));
        break;
    }

    return SUCCESS;
}
