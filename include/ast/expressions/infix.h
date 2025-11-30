#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
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
TRY_STATUS infix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable INFIX_VTABLE = {
    .base =
        {
            .destroy     = infix_expression_destroy,
            .reconstruct = infix_expression_reconstruct,
        },
};
