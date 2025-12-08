#include <assert.h>

#include "ast/statements/jump.h"

NODISCARD Status jump_statement_create(Token           start_token,
                                       Expression*     value,
                                       JumpStatement** jump_stmt,
                                       memory_alloc_fn memory_alloc) {
    assert(memory_alloc);
    switch (start_token.type) {
    case RETURN:
    case BREAK:
    case CONTINUE:
        break;
    default:
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

    JumpStatement* jump = (JumpStatement*)node;
    NODE_VIRTUAL_FREE(jump->value, free_alloc);

    free_alloc(jump);
}

NODISCARD Status jump_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    switch (node->start_token.type) {
    case RETURN:
        TRY(string_builder_append_str_z(sb, "return"));
        break;
    case BREAK:
        TRY(string_builder_append_str_z(sb, "break"));
        break;
    case CONTINUE:
        TRY(string_builder_append_str_z(sb, "continue"));
        break;
    default:
        UNREACHABLE;
        break;
    }

    JumpStatement* jump = (JumpStatement*)node;
    if (jump->value) {
        TRY(string_builder_append(sb, ' '));
        Node* value_node = (Node*)jump->value;
        TRY(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}
