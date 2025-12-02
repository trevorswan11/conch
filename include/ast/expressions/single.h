#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct {
    Expression base;
} NilExpression;

TRY_STATUS
nil_expression_create(Token start_token, NilExpression** nil_expr, memory_alloc_fn memory_alloc);

void nil_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
nil_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable NIL_VTABLE = {
    .base =
        {
            .destroy     = nil_expression_destroy,
            .reconstruct = nil_expression_reconstruct,
        },
};

typedef struct {
    Expression base;
} ContinueExpression;

TRY_STATUS
continue_expression_create(Token                start_token,
                           ContinueExpression** continue_expr,
                           memory_alloc_fn      memory_alloc);

void continue_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
continue_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable CONTINUE_VTABLE = {
    .base =
        {
            .destroy     = continue_expression_destroy,
            .reconstruct = continue_expression_reconstruct,
        },
};
