#include <assert.h>

#include "ast/statements/jump.h"

TRY_STATUS jump_statement_create(Token           start_token,
                                 Expression*     value,
                                 JumpStatement** jump_stmt,
                                 memory_alloc_fn memory_alloc) {
    assert(memory_alloc);
    if (start_token.type != RETURN && start_token.type != BREAK) {
        return UNEXPECTED_TOKEN;
    }

    JumpStatement* jump = memory_alloc(sizeof(JumpStatement));
    if (!jump) {
        return ALLOCATION_FAILED;
    }

    *jump = (JumpStatement){
        .base  = STATEMENT_INIT(JUMP_VTABLE, start_token),
        .value = value,
    };

    *jump_stmt = jump;
    return SUCCESS;
}

void jump_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    JumpStatement* j = (JumpStatement*)node;

    NODE_VIRTUAL_FREE(j->value, free_alloc);
    j->value = NULL;

    free_alloc(j);
}

TRY_STATUS jump_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    if (node->start_token.type == RETURN) {
        PROPAGATE_IF_ERROR(string_builder_append_slice(sb, slice_from_str_z("return")));
    } else if (node->start_token.type == BREAK) {
        PROPAGATE_IF_ERROR(string_builder_append_slice(sb, slice_from_str_z("break")));
    } else {
        UNREACHABLE;
    }

    JumpStatement* j = (JumpStatement*)node;
    if (j->value) {
        PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));
        Node* value_node = (Node*)j->value;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
