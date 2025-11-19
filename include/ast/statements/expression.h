#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Statement   base;
    Token       first_expression_token;
    Expression* expression;
} ExpressionStatement;

TRY_STATUS expression_statement_create(Token                 first_expression_token,
                                       Expression*           expression,
                                       ExpressionStatement** expr_stmt,
                                       memory_alloc_fn       memory_alloc);

void  expression_statement_destroy(Node* node, free_alloc_fn free_alloc);
Slice expression_statement_token_literal(Node* node);
TRY_STATUS
expression_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable EXPR_VTABLE = {
    .base =
        {
            .destroy       = expression_statement_destroy,
            .token_literal = expression_statement_token_literal,
            .reconstruct   = expression_statement_reconstruct,
        },
};
