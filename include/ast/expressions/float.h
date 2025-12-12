#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/status.h"

typedef struct {
    Expression base;
    double     value;
} FloatLiteralExpression;

NODISCARD Status float_literal_expression_create(Token                    start_token,
                                                 double                   value,
                                                 FloatLiteralExpression** float_expr,
                                                 memory_alloc_fn          memory_alloc);

void             float_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status float_literal_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb);
NODISCARD Status float_literal_expression_analyze(Node*            node,
                                                  SemanticContext* parent,
                                                  ArrayList*       errors);

static const ExpressionVTable FLOAT_VTABLE = {
    .base =
        {
            .destroy     = float_literal_expression_destroy,
            .reconstruct = float_literal_expression_reconstruct,
            .analyze     = float_literal_expression_analyze,
        },
};
