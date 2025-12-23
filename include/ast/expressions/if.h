#ifndef IF_EXPR_H
#define IF_EXPR_H

#include <stdint.h>

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

typedef struct IfExpression {
    Expression  base;
    Expression* condition;
    Statement*  consequence;
    Statement*  alternate;
} IfExpression;

NODISCARD Status if_expression_create(Token           start_token,
                                      Expression*     condition,
                                      Statement*      consequence,
                                      Statement*      alternate,
                                      IfExpression**  if_expr,
                                      memory_alloc_fn memory_alloc);

void             if_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status if_expression_reconstruct(Node*          node,
                                           const HashMap* symbol_map,
                                           StringBuilder* sb);
NODISCARD Status if_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable IF_VTABLE = {
    .base =
        {
            .destroy     = if_expression_destroy,
            .reconstruct = if_expression_reconstruct,
            .analyze     = if_expression_analyze,
        },
};

#endif
