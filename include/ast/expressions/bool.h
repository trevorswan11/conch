#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Expression base;
    bool       value;
} BoolLiteralExpression;

TRY_STATUS bool_literal_expression_create(Token                   start_token,
                                          BoolLiteralExpression** bool_expr,
                                          memory_alloc_fn         memory_alloc);

void  bool_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
Slice bool_literal_expression_token_literal(Node* node);
TRY_STATUS
bool_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable BOOL_VTABLE = {
    .base =
        {
            .destroy       = bool_literal_expression_destroy,
            .token_literal = bool_literal_expression_token_literal,
            .reconstruct   = bool_literal_expression_reconstruct,
        },
};
