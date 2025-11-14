#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Expression base;
    Token      token;
    int64_t    value;
} IntegerLiteralExpression;

TRY_STATUS integer_literal_expression_create(Token                      token,
                                             int64_t                    value,
                                             IntegerLiteralExpression** int_expr,
                                             memory_alloc_fn            memory_alloc);

void       integer_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
Slice      integer_literal_expression_token_literal(Node* node);
TRY_STATUS integer_literal_expression_reconstruct(Node* node, StringBuilder* sb);

static const ExpressionVTable INTEGER_VTABLE = {
    .base =
        {
            .destroy       = integer_literal_expression_destroy,
            .token_literal = integer_literal_expression_token_literal,
            .reconstruct   = integer_literal_expression_reconstruct,
        },
};
