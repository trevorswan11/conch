#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/mem.h"

typedef struct {
    Statement             base;
    IdentifierExpression* ident;
    Expression*           value;
} VarStatement;

static const char* var_statement_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return token_type_name(VAR);
}

static void var_statement_destroy(Node* node) {
    VarStatement* variable = (VarStatement*)node;

    if (variable->ident) {
        Node* n_ident = (Node*)variable->ident;
        n_ident->vtable->destroy(n_ident);
        variable->ident = NULL;
    }

    if (variable->value) {
        Node* n_value = (Node*)variable->value;
        n_value->vtable->destroy(n_value);
        variable->value = NULL;
    }

    free(variable);
}

static void var_statement_node(Statement* stat) {
    VarStatement* ident = (VarStatement*)stat;
    MAYBE_UNUSED(ident);
}

static const StatementVTable VAR_VTABLE = {
    .base =
        {
            .token_literal = var_statement_token_literal,
            .destroy       = var_statement_destroy,
        },
    .statement_node = var_statement_node,
};

VarStatement* var_statement_create(IdentifierExpression* ident, Expression* value);
