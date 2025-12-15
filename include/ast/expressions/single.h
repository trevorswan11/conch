#pragma once

#include "ast/expressions/expression.h"

void single_expression_destroy(Node* node, free_alloc_fn free_alloc);

typedef struct NilExpression {
    Expression base;
} NilExpression;

NODISCARD Status nil_expression_create(Token           start_token,
                                       NilExpression** nil_expr,
                                       memory_alloc_fn memory_alloc);

NODISCARD Status nil_expression_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);
NODISCARD Status nil_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

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

NODISCARD Status ignore_expression_create(Token              start_token,
                                          IgnoreExpression** ignore_expr,
                                          memory_alloc_fn    memory_alloc);

NODISCARD Status ignore_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);
NODISCARD Status ignore_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable IGNORE_VTABLE = {
    .base =
        {
            .destroy     = single_expression_destroy,
            .reconstruct = ignore_expression_reconstruct,
            .analyze     = ignore_expression_analyze,
        },
};
