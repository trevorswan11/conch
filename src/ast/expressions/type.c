#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/function.h"
#include "ast/expressions/type.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

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
        switch (type->type.variant.explicit_type.tag) {
        case EXPLICIT_IDENT: {
            IdentifierExpression* ident = type->type.variant.explicit_type.variant.ident_type_name;
            identifier_expression_destroy((Node*)ident, free_alloc);
            ident = NULL;
            break;
        }
        case EXPLICIT_FN: {
            ArrayList params = type->type.variant.explicit_type.variant.fn_type_params;
            free_parameter_list(&params);
            break;
        }
        }
    }

    free_alloc(type);
}

Slice type_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    TypeExpression* type = (TypeExpression*)node;
    if (type->type.tag == EXPLICIT) {
        switch (type->type.variant.explicit_type.tag) {
        case EXPLICIT_IDENT: {
            const MutSlice type_name =
                type->type.variant.explicit_type.variant.ident_type_name->name;
            return slice_from_str_s(type_name.ptr, type_name.length);
        }
        case EXPLICIT_FN:
            return slice_from_str_z(token_type_name(FUNCTION));
        default:
            UNREACHABLE_IF(false);
            return slice_from_str_z("UNREACHABLE");
        }
    }

    return slice_from_str_z(token_type_name(WALRUS));
}

TRY_STATUS
type_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    TypeExpression* type = (TypeExpression*)node;
    if (type->type.tag == EXPLICIT) {
        PROPAGATE_IF_ERROR(string_builder_append_many(sb, ": ", 2));
        if (type->type.variant.explicit_type.nullable) {
            PROPAGATE_IF_ERROR(string_builder_append(sb, '?'));
        }

        switch (type->type.variant.explicit_type.tag) {
        case EXPLICIT_IDENT:
            PROPAGATE_IF_ERROR(string_builder_append_mut_slice(
                sb, type->type.variant.explicit_type.variant.ident_type_name->name));
            break;
        case EXPLICIT_FN: {
            PROPAGATE_IF_ERROR(string_builder_append_many(sb, "fn(", 3));
            ArrayList params = type->type.variant.explicit_type.variant.fn_type_params;
            PROPAGATE_IF_ERROR(reconstruct_parameter_list(&params, symbol_map, sb));
            PROPAGATE_IF_ERROR(string_builder_append(sb, ')'));
            break;
        }
        default:
            UNREACHABLE_IF(false);
        }
    } else {
        PROPAGATE_IF_ERROR(string_builder_append_many(sb, " :", 2));
    }
    return SUCCESS;
}
