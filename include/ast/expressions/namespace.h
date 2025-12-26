#ifndef NAMESPACE_EXPR_H
#define NAMESPACE_EXPR_H

#include "ast/expressions/expression.h"

typedef struct IdentifierExpression IdentifierExpression;

typedef struct NamespaceExpression {
    Expression            base;
    Expression*           outer;
    IdentifierExpression* inner;
} NamespaceExpression;

[[nodiscard]] Status namespace_expression_create(Token                 start_token,
                                             Expression*           outer,
                                             IdentifierExpression* inner,
                                             NamespaceExpression** namespace_expr,
                                             memory_alloc_fn       memory_alloc);

void             namespace_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status namespace_expression_reconstruct(Node*          node,
                                                  const HashMap* symbol_map,
                                                  StringBuilder* sb);
[[nodiscard]] Status namespace_expression_analyze(Node*            node,
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

#endif
