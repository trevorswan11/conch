#ifndef FLOAT_EXPR_H
#define FLOAT_EXPR_H

#include "ast/expressions/expression.h"

typedef struct {
    Expression base;
    double     value;
} FloatLiteralExpression;

[[nodiscard]] Status float_literal_expression_create(Token                    start_token,
                                                     double                   value,
                                                     FloatLiteralExpression** float_expr,
                                                     memory_alloc_fn          memory_alloc);

void float_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status
float_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
float_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable FLOAT_VTABLE = {
    .base =
        {
            .destroy     = float_literal_expression_destroy,
            .reconstruct = float_literal_expression_reconstruct,
            .analyze     = float_literal_expression_analyze,
        },
};

#endif
