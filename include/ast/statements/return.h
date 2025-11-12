#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Statement   base;
    Expression* value;
} ReturnStatement;

TRY_STATUS return_statement_create(Expression* value, ReturnStatement** ret_stmt);

void       return_statement_destroy(Node* node);
Slice      return_statement_token_literal(Node* node);
TRY_STATUS return_statement_reconstruct(Node* node, StringBuilder* sb);
TRY_STATUS return_statement_node(Statement* stmt);

static const StatementVTable RET_VTABLE = {
    .base =
        {
            .destroy       = return_statement_destroy,
            .token_literal = return_statement_token_literal,
            .reconstruct   = return_statement_reconstruct,
        },
    .statement_node = return_statement_node,
};
