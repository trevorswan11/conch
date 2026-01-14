#ifndef ARRAY_EXPR_H
#define ARRAY_EXPR_H

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

typedef struct UnsignedIntegerLiteralExpression UnsignedIntegerLiteralExpression;

typedef struct ArrayLiteralExpression {
    Expression base;
    bool       inferred_size;
    ArrayList  items;
} ArrayLiteralExpression;

[[nodiscard]] Status array_literal_expression_create(Token                    start_token,
                                                     bool                     inferred_size,
                                                     ArrayList                items,
                                                     ArrayLiteralExpression** array_expr,
                                                     Allocator*               allocator);

void array_literal_expression_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
array_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
array_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable ARRAY_VTABLE = {
    .base =
        {
            .destroy     = array_literal_expression_destroy,
            .reconstruct = array_literal_expression_reconstruct,
            .analyze     = array_literal_expression_analyze,
        },
};

#endif
