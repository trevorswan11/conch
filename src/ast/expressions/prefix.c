#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/prefix.h"

TRY_STATUS prefix_expression_create(Token              start_token,
                                    Expression*        rhs,
                                    PrefixExpression** prefix_expr,
                                    memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    if (!rhs) {
        return NULL_PARAMETER;
    }

    PrefixExpression* prefix = memory_alloc(sizeof(PrefixExpression));
    if (!prefix) {
        return ALLOCATION_FAILED;
    }

    *prefix = (PrefixExpression){
        .base = EXPRESSION_INIT(PREFIX_VTABLE, start_token),
        .rhs  = rhs,
    };

    *prefix_expr = prefix;
    return SUCCESS;
}

void prefix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    PrefixExpression* prefix = (PrefixExpression*)node;
    NODE_VIRTUAL_FREE(prefix->rhs, free_alloc);

    free_alloc(prefix);
}

TRY_STATUS prefix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    Slice             op     = poll_tt_symbol(symbol_map, node->start_token.type);
    PrefixExpression* prefix = (PrefixExpression*)node;
    assert(prefix->rhs);
    Node* rhs = (Node*)prefix->rhs;

    PROPAGATE_IF_ERROR(string_builder_append(sb, '('));
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, op));
    PROPAGATE_IF_ERROR(rhs->vtable->reconstruct(rhs, symbol_map, sb));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ')'));

    return SUCCESS;
}
