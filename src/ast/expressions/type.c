#include <assert.h>

#include "ast/expressions/enum.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/struct.h"
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
            NODE_VIRTUAL_FREE(ident, free_alloc);
            break;
        }
        case EXPLICIT_FN: {
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            free_parameter_list(&function_type.fn_type_params);
            NODE_VIRTUAL_FREE(function_type.return_type, free_alloc);
            break;
        }
        case EXPLICIT_STRUCT: {
            StructExpression* s = explicit_type.variant.struct_type;
            NODE_VIRTUAL_FREE(s, free_alloc);
            break;
        }
        case EXPLICIT_ENUM: {
            EnumExpression* e = explicit_type.variant.enum_type;
            NODE_VIRTUAL_FREE(e, free_alloc);
            break;
        }
        case EXPLICIT_ARRAY: {
            TypeExpression* inner = explicit_type.variant.array_type.inner_type;
            array_list_deinit(&explicit_type.variant.array_type.dimensions);
            NODE_VIRTUAL_FREE(inner, free_alloc);
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

        PROPAGATE_IF_ERROR(explicit_type_reconstruct(explicit_type, symbol_map, sb));
    } else {
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " :"));
    }
    return SUCCESS;
}

TRY_STATUS
explicit_type_reconstruct(ExplicitType   explicit_type,
                          const HashMap* symbol_map,
                          StringBuilder* sb) {
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
    case EXPLICIT_STRUCT:
        PROPAGATE_IF_ERROR(struct_expression_reconstruct(
            (Node*)explicit_type.variant.struct_type, symbol_map, sb));
        break;
    case EXPLICIT_ENUM:
        PROPAGATE_IF_ERROR(
            enum_expression_reconstruct((Node*)explicit_type.variant.enum_type, symbol_map, sb));
        break;
    case EXPLICIT_ARRAY:
        PROPAGATE_IF_ERROR(string_builder_append(sb, '['));

        uint64_t dim;
        if (explicit_type.variant.array_type.dimensions.length > 0) {
            for (size_t i = 0; i < explicit_type.variant.array_type.dimensions.length; i++) {
                UNREACHABLE_IF_ERROR(
                    array_list_get(&explicit_type.variant.array_type.dimensions, i, &dim));
                PROPAGATE_IF_ERROR(string_builder_append_unsigned(sb, dim));

                PROPAGATE_IF_ERROR(string_builder_append(sb, 'u'));
                if (i != explicit_type.variant.array_type.dimensions.length - 1) {
                    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, ", "));
                }
            }
        } else {
            UNREACHABLE;
        }

        PROPAGATE_IF_ERROR(string_builder_append(sb, ']'));

        TypeExpression* inner_type = explicit_type.variant.array_type.inner_type;
        assert(inner_type->type.tag == EXPLICIT);

        ExplicitType inner_explicit_type = inner_type->type.variant.explicit_type;
        if (inner_explicit_type.nullable) {
            PROPAGATE_IF_ERROR(string_builder_append(sb, '?'));
        }
        PROPAGATE_IF_ERROR(explicit_type_reconstruct(inner_explicit_type, symbol_map, sb));
        break;
    }

    return SUCCESS;
}
