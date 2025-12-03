#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct PrefixExpression {
    Expression  base;
    Expression* rhs;
} PrefixExpression;

TRY_STATUS prefix_expression_create(Token              start_token,
                                    Expression*        rhs,
                                    PrefixExpression** prefix_expr,
                                    memory_alloc_fn    memory_alloc);

void       prefix_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS prefix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable PREFIX_VTABLE = {
    .base =
        {
            .destroy     = prefix_expression_destroy,
            .reconstruct = prefix_expression_reconstruct,
        },
};
