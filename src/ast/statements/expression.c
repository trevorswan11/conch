#include <assert.h>

#include "ast/statements/expression.h"

TRY_STATUS expression_statement_create(Token                 start_token,
                                       Expression*           expression,
                                       ExpressionStatement** expr_stmt,
                                       memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    if (!expression) {
        return NULL_PARAMETER;
    }

    ExpressionStatement* expr = memory_alloc(sizeof(ExpressionStatement));
    if (!expr) {
        return ALLOCATION_FAILED;
    }

    *expr = (ExpressionStatement){
        .base       = STATEMENT_INIT(EXPR_VTABLE, start_token),
        .expression = expression,
    };

    *expr_stmt = expr;
    return SUCCESS;
}

void expression_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    NODE_VIRTUAL_FREE(expr_stmt->expression, free_alloc);

    free_alloc(expr_stmt);
}

TRY_STATUS
expression_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    ExpressionStatement* expr_stmt  = (ExpressionStatement*)node;
    Node*                value_node = (Node*)expr_stmt->expression;
    PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));

    return SUCCESS;
}
