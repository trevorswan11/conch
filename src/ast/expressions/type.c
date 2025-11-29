#include <assert.h>

#include "ast/expressions/function.h"
#include "ast/expressions/type.h"

TRY_STATUS
type_expression_create(Token            start_token,
                       TypeTag          tag,
                       TypeUnion        variant,
                       TypeExpression** type_expr,
                       memory_alloc_fn  memory_alloc) {
    assert(memory_alloc);
    TypeExpression* type = memory_alloc(sizeof(TypeExpression));
    if (!type) {
        return ALLOCATION_FAILED;
    }

    *type = (TypeExpression){
        .base = EXPRESSION_INIT(TYPE_VTABLE, start_token),
        .type = {tag, variant},
    };

    *type_expr = type;
    return SUCCESS;
}

void type_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    TypeExpression* type = (TypeExpression*)node;

    if (type->type.tag == EXPLICIT) {
        ExplicitType explicit_type = type->type.variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT: {
            IdentifierExpression* ident = explicit_type.variant.ident_type_name;
            identifier_expression_destroy((Node*)ident, free_alloc);
            ident = NULL;
            break;
        }
        case EXPLICIT_FN: {
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            free_parameter_list(&function_type.fn_type_params);
            type_expression_destroy((Node*)function_type.return_type, free_alloc);
            break;
        }
        }
    }

    free_alloc(type);
}

TRY_STATUS
type_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    TypeExpression* type = (TypeExpression*)node;
    if (type->type.tag == EXPLICIT) {
        ExplicitType explicit_type = type->type.variant.explicit_type;
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, ": "));
        if (explicit_type.nullable) {
            PROPAGATE_IF_ERROR(string_builder_append(sb, '?'));
        }

        switch (explicit_type.tag) {
        case EXPLICIT_IDENT:
            PROPAGATE_IF_ERROR(
                string_builder_append_mut_slice(sb, explicit_type.variant.ident_type_name->name));
            break;
        case EXPLICIT_FN: {
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "fn("));
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            PROPAGATE_IF_ERROR(
                reconstruct_parameter_list(&function_type.fn_type_params, symbol_map, sb));
            PROPAGATE_IF_ERROR(string_builder_append(sb, ')'));
            PROPAGATE_IF_ERROR(
                type_expression_reconstruct((Node*)function_type.return_type, symbol_map, sb));
            break;
        }
        default:
            UNREACHABLE;
        }
    } else {
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " :"));
    }
    return SUCCESS;
}
