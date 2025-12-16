#include <assert.h>

#include "ast/statements/expression.h"

#include "ast/node.h"
#include "semantic/context.h"

#include "util/containers/string_builder.h"

NODISCARD Status expression_statement_create(Token                 start_token,
                                             Expression*           expression,
                                             ExpressionStatement** expr_stmt,
                                             memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(expression);

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
    if (!node) {
        return;
    }
    assert(free_alloc);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    NODE_VIRTUAL_FREE(expr_stmt->expression, free_alloc);

    free_alloc(expr_stmt);
}

NODISCARD Status expression_statement_reconstruct(Node*          node,
                                                  const HashMap* symbol_map,
                                                  StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    ASSERT_EXPRESSION(expr_stmt->expression);
    return NODE_VIRTUAL_RECONSTRUCT(expr_stmt->expression, symbol_map, sb);
}

NODISCARD Status expression_statement_analyze(Node*            node,
                                              SemanticContext* parent,
                                              ArrayList*       errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    ASSERT_EXPRESSION(expr_stmt->expression);

    return NODE_VIRTUAL_ANALYZE(expr_stmt->expression, parent, errors);
}
