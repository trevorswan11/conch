#ifndef ASSIGNMENT_EXPR_H
#define ASSIGNMENT_EXPR_H

#include "ast/expressions/expression.h"

// Semantically different compared to generic infix.
//
// Parser invariant allows easy analysis.
typedef struct AssignmentExpression {
    Expression  base;
    Expression* lhs;
    TokenType   op;
    Expression* rhs;
} AssignmentExpression;

NODISCARD Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
                                              TokenType              op,
                                              Expression*            rhs,
                                              AssignmentExpression** assignment_expr,
                                              memory_alloc_fn        memory_alloc);

void             assignment_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status assignment_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb);
NODISCARD Status assignment_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors);

static const ExpressionVTable ASSIGNMENT_VTABLE = {
    .base =
        {
            .destroy     = assignment_expression_destroy,
            .reconstruct = assignment_expression_reconstruct,
            .analyze     = assignment_expression_analyze,
        },
};

#endif
