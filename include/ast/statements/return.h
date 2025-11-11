#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/error.h"
#include "util/mem.h"

typedef struct {
    Statement   base;
    Expression* value;
} ReturnStatement;

static const char* return_statement_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return token_type_name(RETURN);
}

static void return_statement_destroy(Node* node) {
    ReturnStatement* r = (ReturnStatement*)node;

    if (r->value) {
        Node* n_value = (Node*)r->value;
        n_value->vtable->destroy(n_value);
        r->value = NULL;
    }

    free(r);
}

static void return_statement_node(Statement* stat) {
    MAYBE_UNUSED(stat);
}

static const StatementVTable RET_VTABLE = {
    .base =
        {
            .token_literal = return_statement_token_literal,
            .destroy       = return_statement_destroy,
        },
    .statement_node = return_statement_node,
};

AnyError return_statement_create(Expression* value, ReturnStatement** ret_stmt);
