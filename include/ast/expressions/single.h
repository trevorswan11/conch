#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

void single_expression_destroy(Node* node, free_alloc_fn free_alloc);

typedef struct NilExpression {
    Expression base;
} NilExpression;

NODISCARD Status nil_expression_create(Token           start_token,
                                       NilExpression** nil_expr,
                                       memory_alloc_fn memory_alloc);

NODISCARD Status nil_expression_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);

static const ExpressionVTable NIL_VTABLE = {
    .base =
        {
            .destroy     = single_expression_destroy,
            .reconstruct = nil_expression_reconstruct,
        },
};

typedef struct IgnoreExpression {
    Expression base;
} IgnoreExpression;

NODISCARD Status ignore_expression_create(Token              start_token,
                                          IgnoreExpression** ignore_expr,
                                          memory_alloc_fn    memory_alloc);

NODISCARD Status ignore_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);

static const ExpressionVTable IGNORE_VTABLE = {
    .base =
        {
            .destroy     = single_expression_destroy,
            .reconstruct = ignore_expression_reconstruct,
        },
};
