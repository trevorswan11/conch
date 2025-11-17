#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/return.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

TRY_STATUS return_statement_create(Expression*       value,
                                   ReturnStatement** ret_stmt,
                                   memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    ReturnStatement* ret = memory_alloc(sizeof(ReturnStatement));
    if (!ret) {
        return ALLOCATION_FAILED;
    }

    *ret = (ReturnStatement){
        .base  = STATEMENT_INIT(RET_VTABLE),
        .value = value,
    };

    *ret_stmt = ret;
    return SUCCESS;
}

void return_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    ReturnStatement* r = (ReturnStatement*)node;

    if (r->value) {
        Node* n_value = (Node*)r->value;
        n_value->vtable->destroy(n_value, free_alloc);
        r->value = NULL;
    }

    free_alloc(r);
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
