#include <assert.h>

#include "ast/statements/discard.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status discard_statement_create(Token              start_token,
                                              Expression*        to_discard,
                                              DiscardStatement** discard_stmt,
                                              Allocator*         allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(to_discard);

    DiscardStatement* discard = ALLOCATOR_PTR_MALLOC(allocator, sizeof(DiscardStatement));
    if (!discard) { return ALLOCATION_FAILED; }

    *discard = (DiscardStatement){
        .base       = STATEMENT_INIT(DISCARD_VTABLE, start_token),
        .to_discard = to_discard,
    };

    *discard_stmt = discard;
    return SUCCESS;
}

void discard_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    DiscardStatement* discard = (DiscardStatement*)node;
    NODE_VIRTUAL_FREE(discard->to_discard, allocator);

    ALLOCATOR_PTR_FREE(allocator, discard);
}

[[nodiscard]] Status
discard_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "_ = "));

    DiscardStatement* discard = (DiscardStatement*)node;
    ASSERT_EXPRESSION(discard->to_discard);
    TRY(NODE_VIRTUAL_RECONSTRUCT(discard->to_discard, symbol_map, sb));

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status
discard_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    Allocator*        allocator = semantic_context_allocator(parent);
    DiscardStatement* discard   = (DiscardStatement*)node;
    ASSERT_EXPRESSION(discard->to_discard);

    TRY(NODE_VIRTUAL_ANALYZE(discard->to_discard, parent, errors));
    SemanticType* discarded = semantic_context_move_analyzed(parent);
    RC_RELEASE(discarded, allocator);
    return SUCCESS;
}
