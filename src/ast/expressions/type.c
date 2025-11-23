#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/type.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS
type_expression_create(TypeTag          tag,
                       TypeUnion        onion,
                       TypeExpression** type_expr,
                       memory_alloc_fn  memory_alloc) {
    assert(memory_alloc);
    TypeExpression* type = memory_alloc(sizeof(TypeExpression));
    if (!type) {
        return ALLOCATION_FAILED;
    }

    *type = (TypeExpression){
        .base = EXPRESSION_INIT(TYPE_VTABLE),
        .type = {tag, onion},
    };

    *type_expr = type;
    return SUCCESS;
}

void type_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    TypeExpression* type = (TypeExpression*)node;

    if (type->type.tag == EXPLICIT && type->type.variant.explicit_type.identifier) {
        IdentifierExpression* ident      = type->type.variant.explicit_type.identifier;
        Node*                 ident_node = (Node*)ident;
        ident_node->vtable->destroy(ident_node, free_alloc);
    }

    free_alloc(type);
}

Slice type_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    TypeExpression* type = (TypeExpression*)node;
    if (type->type.tag == EXPLICIT) {
        const MutSlice type_name = type->type.variant.explicit_type.identifier->name;
        return slice_from_str_s(type_name.ptr, type_name.length);
    } else {
        return slice_from_str_z(token_type_name(WALRUS));
    }
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
        PROPAGATE_IF_ERROR(
            string_builder_append_mut_slice(sb, type->type.variant.explicit_type.identifier->name));
    } else {
        PROPAGATE_IF_ERROR(string_builder_append_many(sb, " :", 2));
    }
    return SUCCESS;
}
