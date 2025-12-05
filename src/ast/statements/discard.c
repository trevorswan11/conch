#include <assert.h>

#include "ast/statements/discard.h"

TRY_STATUS discard_statement_create(Token              start_token,
                                    Expression*        to_discard,
                                    DiscardStatement** discard_stmt,
                                    memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    if (!to_discard) {
        return NULL_PARAMETER;
    }

    DiscardStatement* discard = memory_alloc(sizeof(DiscardStatement));
    if (!discard) {
        return ALLOCATION_FAILED;
    }

    *discard = (DiscardStatement){
        .base       = STATEMENT_INIT(DISCARD_VTABLE, start_token),
        .to_discard = to_discard,
    };

    *discard_stmt = discard;
    return SUCCESS;
}

void discard_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    DiscardStatement* discard = (DiscardStatement*)node;
    NODE_VIRTUAL_FREE(discard->to_discard, free_alloc);

    free_alloc(discard);
}

TRY_STATUS
discard_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "_ = "));

    DiscardStatement* discard    = (DiscardStatement*)node;
    Node*             to_discard = (Node*)discard->to_discard;
    PROPAGATE_IF_ERROR(to_discard->vtable->reconstruct(to_discard, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
