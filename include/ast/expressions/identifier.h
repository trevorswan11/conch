#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Expression base;
    MutSlice   name;
} IdentifierExpression;

TRY_STATUS identifier_expression_create(Slice                  name,
                                        IdentifierExpression** ident_expr,
                                        memory_alloc_fn        memory_alloc,
                                        free_alloc_fn          free_alloc);

void       identifier_expression_destroy(Node* node, free_alloc_fn free_alloc);
Slice      identifier_expression_token_literal(Node* node);
TRY_STATUS identifier_expression_reconstruct(Node* node, StringBuilder* sb);
TRY_STATUS identifier_expression_node(Expression* expr);

static const ExpressionVTable IDENTIFIER_VTABLE = {
    .base =
        {
            .destroy       = identifier_expression_destroy,
            .token_literal = identifier_expression_token_literal,
            .reconstruct   = identifier_expression_reconstruct,
        },
    .expression_node = identifier_expression_node,
};
