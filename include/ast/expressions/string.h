#ifndef STRING_EXPR_H
#define STRING_EXPR_H

#include "ast/expressions/expression.h"

typedef struct StringLiteralExpression {
    Expression base;
    MutSlice   slice;
} StringLiteralExpression;

[[nodiscard]] Status string_literal_expression_create(Token                     start_token,
                                                  StringLiteralExpression** string_expr,
                                                  Allocator                 allocator);

void             string_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status string_literal_expression_reconstruct(Node*          node,
                                                       const HashMap* symbol_map,
                                                       StringBuilder* sb);
[[nodiscard]] Status string_literal_expression_analyze(Node*            node,
                                                   SemanticContext* parent,
                                                   ArrayList*       errors);

static const ExpressionVTable STRING_VTABLE = {
    .base =
        {
            .destroy     = string_literal_expression_destroy,
            .reconstruct = string_literal_expression_reconstruct,
            .analyze     = string_literal_expression_analyze,
        },
};

#endif
