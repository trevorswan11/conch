#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/return.h"
#include "util/containers/string_builder.h"

#include "util/status.h"

TRY_STATUS return_statement_create(Expression* value, ReturnStatement** ret_stmt) {
    ReturnStatement* ret = malloc(sizeof(ReturnStatement));
    if (!ret) {
        return ALLOCATION_FAILED;
    }

    *ret = (ReturnStatement){
        .base =
            (Statement){
                .base =
                    (Node){
                        .vtable = &RET_VTABLE.base,
                    },
                .vtable = &RET_VTABLE,
            },
        .value = value,
    };

    *ret_stmt = ret;
    return SUCCESS;
}

void return_statement_destroy(Node* node) {
    ASSERT_NODE(node);
    ReturnStatement* r = (ReturnStatement*)node;

    if (r->value) {
        Node* n_value = (Node*)r->value;
        n_value->vtable->destroy(n_value);
        r->value = NULL;
    }

    free(r);
}

Slice return_statement_token_literal(Node* node) {
    ASSERT_NODE(node);
    MAYBE_UNUSED(node);

    return (Slice){
        .ptr    = "return",
        .length = sizeof("return") - 1,
    };
}

TRY_STATUS return_statement_reconstruct(Node* node, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, node->vtable->token_literal(node)));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));

    ReturnStatement* r = (ReturnStatement*)node;
    if (r->value) {
        Node* value_node = (Node*)r->value;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}

TRY_STATUS return_statement_node(Statement* stmt) {
    ASSERT_STATEMENT(stmt);
    MAYBE_UNUSED(stmt);
    return SUCCESS;
}
