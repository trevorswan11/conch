#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct IntegerLiteralExpression {
    Expression base;
    int64_t    value;
} IntegerLiteralExpression;

NODISCARD Status integer_literal_expression_create(Token                      start_token,
                                                   int64_t                    value,
                                                   IntegerLiteralExpression** int_expr,
                                                   memory_alloc_fn            memory_alloc);

void             integer_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status integer_literal_expression_reconstruct(Node*          node,
                                                        const HashMap* symbol_map,
                                                        StringBuilder* sb);

static const ExpressionVTable INTEGER_VTABLE = {
    .base =
        {
            .destroy     = integer_literal_expression_destroy,
            .reconstruct = integer_literal_expression_reconstruct,
        },
};

typedef struct UnsignedIntegerLiteralExpression {
    Expression base;
    uint64_t   value;
} UnsignedIntegerLiteralExpression;

NODISCARD Status uinteger_literal_expression_create(Token                              start_token,
                                                    uint64_t                           value,
                                                    UnsignedIntegerLiteralExpression** int_expr,
                                                    memory_alloc_fn memory_alloc);

void uinteger_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);

static const ExpressionVTable UNSIGNED_INTEGER_VTABLE = {
    .base =
        {
            .destroy     = uinteger_literal_expression_destroy,
            .reconstruct = integer_literal_expression_reconstruct,
        },
};
