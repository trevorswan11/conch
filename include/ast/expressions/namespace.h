#pragma once

#include "ast/expressions/expression.h"

typedef struct IdentifierExpression IdentifierExpression;

typedef struct NamespaceExpression {
    Expression            base;
    Expression*           outer;
    IdentifierExpression* inner;
} NamespaceExpression;

NODISCARD Status namespace_expression_create(Token                 start_token,
                                             Expression*           outer,
                                             IdentifierExpression* inner,
                                             NamespaceExpression** namespace_expr,
                                             memory_alloc_fn       memory_alloc);

void             namespace_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status namespace_expression_reconstruct(Node*          node,
                                                  const HashMap* symbol_map,
                                                  StringBuilder* sb);
NODISCARD Status namespace_expression_analyze(Node*            node,
                                              SemanticContext* parent,
                                              ArrayList*       errors);

static const ExpressionVTable NAMESPACE_VTABLE = {
    .base =
        {
            .destroy     = namespace_expression_destroy,
            .reconstruct = namespace_expression_reconstruct,
            .analyze     = namespace_expression_analyze,
        },
};
