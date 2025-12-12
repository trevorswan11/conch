#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/status.h"

typedef struct InfixExpression {
    Expression  base;
    Expression* lhs;
    TokenType   op;
    Expression* rhs;
} InfixExpression;

NODISCARD Status infix_expression_create(Token             start_token,
                                         Expression*       lhs,
                                         TokenType         op,
                                         Expression*       rhs,
                                         InfixExpression** infix_expr,
                                         memory_alloc_fn   memory_alloc);

void             infix_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status infix_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb);
NODISCARD Status infix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable INFIX_VTABLE = {
    .base =
        {
            .destroy     = infix_expression_destroy,
            .reconstruct = infix_expression_reconstruct,
            .analyze     = infix_expression_analyze,
        },
};
