#include <assert.h>

#include "ast/statements/expression.h"

#include "ast/node.h"
#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status expression_statement_create(Token                 start_token,
                                                 Expression*           expression,
                                                 ExpressionStatement** expr_stmt,
                                                 Allocator*            allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(expression);

    ExpressionStatement* expr = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*expr));
    if (!expr) { return ALLOCATION_FAILED; }

    *expr = (ExpressionStatement){
        .base       = STATEMENT_INIT(EXPR_VTABLE, start_token),
        .expression = expression,
    };

    *expr_stmt = expr;
    return SUCCESS;
}

void expression_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    NODE_VIRTUAL_FREE(expr_stmt->expression, allocator);

    ALLOCATOR_PTR_FREE(allocator, expr_stmt);
}

[[nodiscard]] Status
expression_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    ASSERT_EXPRESSION(expr_stmt->expression);
    return NODE_VIRTUAL_RECONSTRUCT(expr_stmt->expression, symbol_map, sb);
}

[[nodiscard]] Status
expression_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    ASSERT_EXPRESSION(expr_stmt->expression);

    return NODE_VIRTUAL_ANALYZE(expr_stmt->expression, parent, errors);
}
