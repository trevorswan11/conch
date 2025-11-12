#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Statement             base;
    Token                 token;
    IdentifierExpression* ident;
    Expression*           value;
} DeclStatement;

TRY_STATUS decl_statement_create(Token                 token,
                                 IdentifierExpression* ident,
                                 Expression*           value,
                                 DeclStatement**       decl_stmt);

void       decl_statement_destroy(Node* node);
Slice      decl_statement_token_literal(Node* node);
TRY_STATUS decl_statement_reconstruct(Node* node, StringBuilder* sb);
TRY_STATUS decl_statement_node(Statement* stmt);

static const StatementVTable DECL_VTABLE = {
    .base =
        {
            .destroy       = decl_statement_destroy,
            .token_literal = decl_statement_token_literal,
            .reconstruct   = decl_statement_reconstruct,
        },
    .statement_node = decl_statement_node,
};
