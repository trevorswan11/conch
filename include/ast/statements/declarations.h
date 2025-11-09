#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/mem.h"

typedef struct {
    Statement                   base;
    const IdentifierExpression* ident;
    Expression                  value;
} VarStatement;

static const char* var_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return token_type_name(VAR);
}

static void var_destroy(Node* node) {
    VarStatement* variable = (VarStatement*)node;
    free(variable);
}

static void var_statement_node(Statement* stat) {
    VarStatement* ident = (VarStatement*)stat;
    MAYBE_UNUSED(ident);
}

static const StatementVTable VAR_VTABLE = {
    .base =
        {
            .token_literal = var_token_literal,
            .destroy       = var_destroy,
        },
    .statement_node = var_statement_node,
};

VarStatement* var_statement_new(const IdentifierExpression* ident, Expression value);
