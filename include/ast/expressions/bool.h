#ifndef BOOL_EXPR_H
#define BOOL_EXPR_H

#include "ast/expressions/expression.h"

typedef struct BoolLiteralExpression {
    Expression base;
    bool       value;
} BoolLiteralExpression;

[[nodiscard]] Status bool_literal_expression_create(Token                   start_token,
                                                    BoolLiteralExpression** bool_expr,
                                                    memory_alloc_fn         memory_alloc);

void bool_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status
bool_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
bool_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable BOOL_VTABLE = {
    .base =
        {
            .destroy     = bool_literal_expression_destroy,
            .reconstruct = bool_literal_expression_reconstruct,
            .analyze     = bool_literal_expression_analyze,
        },
};

#endif
