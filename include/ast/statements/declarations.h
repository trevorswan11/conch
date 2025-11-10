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
    bool                  constant;
} DeclStatement;

static const char* decl_statement_token_literal(Node* node) {
    DeclStatement* d = (DeclStatement*)node;
    return d->constant ? token_type_name(CONST) : token_type_name(VAR);
}

static void decl_statement_destroy(Node* node) {
    DeclStatement* d = (DeclStatement*)node;

    if (d->ident) {
        Node* n_ident = (Node*)d->ident;
        n_ident->vtable->destroy(n_ident);
        d->ident = NULL;
    }

    if (d->value) {
        Node* n_value = (Node*)d->value;
        n_value->vtable->destroy(n_value);
        d->value = NULL;
    }

    free(d);
}

static void decl_statement_node(Statement* stat) {
    MAYBE_UNUSED(stat);
}

static const StatementVTable DECL_VTABLE = {
    .base =
        {
            .token_literal = decl_statement_token_literal,
            .destroy       = decl_statement_destroy,
        },
    .statement_node = decl_statement_node,
};

DeclStatement* decl_statement_create(IdentifierExpression* ident, Expression* value, bool constant);
