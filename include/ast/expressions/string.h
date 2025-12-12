#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct StringLiteralExpression {
    Expression base;
    MutSlice   slice;
} StringLiteralExpression;

NODISCARD Status string_literal_expression_create(Token                     start_token,
                                                  StringLiteralExpression** string_expr,
                                                  Allocator                 allocator);

void             string_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status string_literal_expression_reconstruct(Node*          node,
                                                       const HashMap* symbol_map,
                                                       StringBuilder* sb);
NODISCARD Status string_literal_expression_analyze(Node*            node,
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
