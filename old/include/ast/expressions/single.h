#ifndef SINGLE_EXPR_H
#define SINGLE_EXPR_H

#include "ast/expressions/expression.h"

void single_expression_destroy(Node* node, Allocator* allocator);

typedef struct NilExpression {
    Expression base;
} NilExpression;

[[nodiscard]] Status
nil_expression_create(Token start_token, NilExpression** nil_expr, Allocator* allocator);

[[nodiscard]] Status
nil_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status nil_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable NIL_VTABLE = {
    .base =
        {
            .destroy     = single_expression_destroy,
            .reconstruct = nil_expression_reconstruct,
            .analyze     = nil_expression_analyze,
        },
};

typedef struct IgnoreExpression {
    Expression base;
} IgnoreExpression;

[[nodiscard]] Status
ignore_expression_create(Token start_token, IgnoreExpression** ignore_expr, Allocator* allocator);

[[nodiscard]] Status
ignore_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
ignore_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable IGNORE_VTABLE = {
    .base =
        {
            .destroy     = single_expression_destroy,
            .reconstruct = ignore_expression_reconstruct,
            .analyze     = ignore_expression_analyze,
        },
};

#endif
