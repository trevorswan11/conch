#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Expression  base;
    Expression* lhs;
    TokenType   op;
    Expression* rhs;
} InfixExpression;

TRY_STATUS infix_expression_create(Token             start_token,
                                   Expression*       lhs,
                                   TokenType         op,
                                   Expression*       rhs,
                                   InfixExpression** infix_expr,
                                   memory_alloc_fn   memory_alloc);

void       infix_expression_destroy(Node* node, free_alloc_fn free_alloc);
Slice      infix_expression_token_literal(Node* node);
TRY_STATUS infix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable INFIX_VTABLE = {
    .base =
        {
            .destroy       = infix_expression_destroy,
            .token_literal = infix_expression_token_literal,
            .reconstruct   = infix_expression_reconstruct,
        },
};
