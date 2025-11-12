#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/expression.h"
#include "util/containers/string_builder.h"

#include "util/status.h"

TRY_STATUS expression_statement_create(Token                 first_expression_token,
                                       Expression*           expression,
                                       ExpressionStatement** expr_stmt) {
    ExpressionStatement* expr = malloc(sizeof(ExpressionStatement));
    if (!expr) {
        return ALLOCATION_FAILED;
    }

    *expr = (ExpressionStatement){
        .base =
            (Statement){
                .base =
                    (Node){
                        .vtable = &EXPR_VTABLE.base,
                    },
                .vtable = &EXPR_VTABLE,
            },
        .first_expression_token = first_expression_token,
        .expression             = expression,
    };

    *expr_stmt = expr;
    return SUCCESS;
}

void expression_statement_destroy(Node* node) {
    ASSERT_NODE(node);
    ExpressionStatement* e = (ExpressionStatement*)node;

    if (e->expression) {
        Node* n_value = (Node*)e->expression;
        n_value->vtable->destroy(n_value);
        e->expression = NULL;
    }

    free(e);
}

Slice expression_statement_token_literal(Node* node) {
    ASSERT_NODE(node);
    MAYBE_UNUSED(node);

    ExpressionStatement* e = (ExpressionStatement*)node;
    return e->first_expression_token.slice;
}

TRY_STATUS expression_statement_reconstruct(Node* node, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    ExpressionStatement* e = (ExpressionStatement*)node;
    if (e->expression) {
        Node* value_node = (Node*)e->expression;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, sb));
    }

    return SUCCESS;
}

TRY_STATUS expression_statement_node(Statement* stmt) {
    ASSERT_STATEMENT(stmt);
    MAYBE_UNUSED(stmt);
    return SUCCESS;
}
