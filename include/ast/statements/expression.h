#ifndef EXPRESSION_STMT_H
#define EXPRESSION_STMT_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

typedef struct ExpressionStatement {
    Statement   base;
    Expression* expression;
} ExpressionStatement;

[[nodiscard]] Status expression_statement_create(Token                 start_token,
                                             Expression*           expression,
                                             ExpressionStatement** expr_stmt,
                                             memory_alloc_fn       memory_alloc);

void             expression_statement_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status expression_statement_reconstruct(Node*          node,
                                                  const HashMap* symbol_map,
                                                  StringBuilder* sb);
[[nodiscard]] Status expression_statement_analyze(Node*            node,
                                              SemanticContext* parent,
                                              ArrayList*       errors);

static const StatementVTable EXPR_VTABLE = {
    .base =
        {
            .destroy     = expression_statement_destroy,
            .reconstruct = expression_statement_reconstruct,
            .analyze     = expression_statement_analyze,
        },
};

#endif
