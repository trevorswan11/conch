#pragma once

#include <stdint.h>

#include "ast/expressions/expression.h"

void integer_expression_destroy(Node* node, free_alloc_fn free_alloc);

NODISCARD Status integer_expression_reconstruct(Node*          node,
                                                const HashMap* symbol_map,
                                                StringBuilder* sb);

typedef struct IntegerLiteralExpression {
    Expression base;
    int64_t    value;
} IntegerLiteralExpression;

NODISCARD Status integer_literal_expression_create(Token                      start_token,
                                                   int64_t                    value,
                                                   IntegerLiteralExpression** int_expr,
                                                   memory_alloc_fn            memory_alloc);

NODISCARD Status integer_literal_expression_analyze(Node*            node,
                                                    SemanticContext* parent,
                                                    ArrayList*       errors);

static const ExpressionVTable INTEGER_VTABLE = {
    .base =
        {
            .destroy     = integer_expression_destroy,
            .reconstruct = integer_expression_reconstruct,
            .analyze     = integer_literal_expression_analyze,
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

NODISCARD Status uinteger_literal_expression_analyze(Node*            node,
                                                     SemanticContext* parent,
                                                     ArrayList*       errors);

static const ExpressionVTable UINTEGER_VTABLE = {
    .base =
        {
            .destroy     = integer_expression_destroy,
            .reconstruct = integer_expression_reconstruct,
            .analyze     = uinteger_literal_expression_analyze,
        },
};

typedef struct SizeIntegerLiteralExpression {
    Expression base;
    size_t     value;
} SizeIntegerLiteralExpression;

NODISCARD Status uzinteger_literal_expression_create(Token                          start_token,
                                                     size_t                         value,
                                                     SizeIntegerLiteralExpression** int_expr,
                                                     memory_alloc_fn                memory_alloc);

NODISCARD Status uzinteger_literal_expression_analyze(Node*            node,
                                                      SemanticContext* parent,
                                                      ArrayList*       errors);

static const ExpressionVTable UZINTEGER_VTABLE = {
    .base =
        {
            .destroy     = integer_expression_destroy,
            .reconstruct = integer_expression_reconstruct,
            .analyze     = uzinteger_literal_expression_analyze,
        },
};

typedef struct ByteLiteralExpression {
    Expression base;
    uint8_t    value;
} ByteLiteralExpression;

NODISCARD Status byte_literal_expression_create(Token                   start_token,
                                                uint8_t                 value,
                                                ByteLiteralExpression** byte_expr,
                                                memory_alloc_fn         memory_alloc);

NODISCARD Status byte_literal_expression_analyze(Node*            node,
                                                 SemanticContext* parent,
                                                 ArrayList*       errors);

static const ExpressionVTable BYTE_VTABLE = {
    .base =
        {
            .destroy     = integer_expression_destroy,
            .reconstruct = integer_expression_reconstruct,
            .analyze     = byte_literal_expression_analyze,
        },
};
