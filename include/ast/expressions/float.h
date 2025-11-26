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
    double     value;
} FloatLiteralExpression;

TRY_STATUS float_literal_expression_create(Token                    start_token,
                                           double                   value,
                                           FloatLiteralExpression** float_expr,
                                           memory_alloc_fn          memory_alloc);

void float_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
float_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable FLOAT_VTABLE = {
    .base =
        {
            .destroy     = float_literal_expression_destroy,
            .reconstruct = float_literal_expression_reconstruct,
        },
};
