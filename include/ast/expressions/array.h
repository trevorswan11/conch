#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct UnsignedIntegerLiteralExpression UnsignedIntegerLiteralExpression;

typedef struct ArrayLiteralExpression {
    Expression base;
    bool       inferred_size;
    ArrayList  items;
} ArrayLiteralExpression;

NODISCARD Status array_literal_expression_create(Token                    start_token,
                                                 bool                     inferred_size,
                                                 ArrayList                items,
                                                 ArrayLiteralExpression** array_expr,
                                                 memory_alloc_fn          memory_alloc);

void             array_literal_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status array_literal_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb);

static const ExpressionVTable ARRAY_VTABLE = {
    .base =
        {
            .destroy     = array_literal_expression_destroy,
            .reconstruct = array_literal_expression_reconstruct,
        },
};
