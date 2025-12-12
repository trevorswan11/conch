#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/status.h"

typedef struct PrefixExpression {
    Expression  base;
    Expression* rhs;
} PrefixExpression;

NODISCARD Status prefix_expression_create(Token              start_token,
                                          Expression*        rhs,
                                          PrefixExpression** prefix_expr,
                                          memory_alloc_fn    memory_alloc);

void             prefix_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status prefix_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);
NODISCARD Status prefix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable PREFIX_VTABLE = {
    .base =
        {
            .destroy     = prefix_expression_destroy,
            .reconstruct = prefix_expression_reconstruct,
            .analyze     = prefix_expression_analyze,
        },
};
