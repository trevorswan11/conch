#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/expression.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

TRY_STATUS expression_statement_create(Token                 first_expression_token,
                                       Expression*           expression,
                                       ExpressionStatement** expr_stmt,
                                       memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    if (!expression) {
        return NULL_PARAMETER;
    }

    ExpressionStatement* expr = memory_alloc(sizeof(ExpressionStatement));
    if (!expr) {
        return ALLOCATION_FAILED;
    }

    *expr = (ExpressionStatement){
        .base                   = STATEMENT_INIT(EXPR_VTABLE),
        .first_expression_token = first_expression_token,
        .expression             = expression,
    };

    *expr_stmt = expr;
    return SUCCESS;
}

void expression_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    ExpressionStatement* e = (ExpressionStatement*)node;

    NODE_VIRTUAL_FREE(e->expression, free_alloc);
    e->expression = NULL;

    free_alloc(e);
}

Slice expression_statement_token_literal(Node* node) {
    ASSERT_NODE(node);
    ExpressionStatement* e = (ExpressionStatement*)node;
    return e->first_expression_token.slice;
}

TRY_STATUS
expression_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    ExpressionStatement* e = (ExpressionStatement*)node;
    if (e->expression) {
        Node* value_node = (Node*)e->expression;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    return SUCCESS;
}
