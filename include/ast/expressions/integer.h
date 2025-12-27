#ifndef INTEGER_EXPR_H
#define INTEGER_EXPR_H

#include <stdint.h>

#include "ast/expressions/expression.h"

void integer_expression_destroy(Node* node, Allocator* allocator);

[[nodiscard]] Status
integer_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

typedef struct IntegerLiteralExpression {
    Expression base;
    int64_t    value;
} IntegerLiteralExpression;

[[nodiscard]] Status integer_literal_expression_create(Token                      start_token,
                                                       int64_t                    value,
                                                       IntegerLiteralExpression** int_expr,
                                                       Allocator*                 allocator);

[[nodiscard]] Status
integer_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

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

[[nodiscard]] Status uinteger_literal_expression_create(Token    start_token,
                                                        uint64_t value,
                                                        UnsignedIntegerLiteralExpression** int_expr,
                                                        Allocator* allocator);

[[nodiscard]] Status
uinteger_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

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

[[nodiscard]] Status uzinteger_literal_expression_create(Token                          start_token,
                                                         size_t                         value,
                                                         SizeIntegerLiteralExpression** int_expr,
                                                         Allocator*                     allocator);

[[nodiscard]] Status
uzinteger_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

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

[[nodiscard]] Status byte_literal_expression_create(Token                   start_token,
                                                    uint8_t                 value,
                                                    ByteLiteralExpression** byte_expr,
                                                    Allocator*              allocator);

[[nodiscard]] Status
byte_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable BYTE_VTABLE = {
    .base =
        {
            .destroy     = integer_expression_destroy,
            .reconstruct = integer_expression_reconstruct,
            .analyze     = byte_literal_expression_analyze,
        },
};

#endif
