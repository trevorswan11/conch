#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct IdentifierExpression IdentifierExpression;

typedef struct NarrowExpression {
    Expression            base;
    Expression*           outer;
    IdentifierExpression* inner;
} NarrowExpression;

NODISCARD Status narrow_expression_create(Token                 start_token,
                                          Expression*           outer,
                                          IdentifierExpression* inner,
                                          NarrowExpression**    narrow_expr,
                                          memory_alloc_fn       memory_alloc);

void             narrow_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status narrow_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);

static const ExpressionVTable NARROW_VTABLE = {
    .base =
        {
            .destroy     = narrow_expression_destroy,
            .reconstruct = narrow_expression_reconstruct,
        },
};
