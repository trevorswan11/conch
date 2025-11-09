#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/mem.h"

typedef struct {
    Expression base;
    MutSlice   name;
} IdentifierExpression;

static const char* identifier_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return token_type_name(IDENT);
}

static void identifier_destroy(Node* node) {
    IdentifierExpression* ident = (IdentifierExpression*)node;
    free(ident->name.ptr);
    free(ident);
}

static void identifier_expression_node(Expression* expr) {
    IdentifierExpression* ident = (IdentifierExpression*)expr;
    MAYBE_UNUSED(ident);
}

static const ExpressionVTable IDENTIFIER_VTABLE = {
    .base =
        {
            .token_literal = identifier_token_literal,
            .destroy       = identifier_destroy,
        },
    .expression_node = identifier_expression_node,
};

IdentifierExpression* identifier_expression_new(const char* name);
