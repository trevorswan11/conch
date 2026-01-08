#include <assert.h>

#include "lexer/token.h"

#include "ast/statements/jump.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status jump_statement_create(Token           start_token,
                                           Expression*     value,
                                           JumpStatement** jump_stmt,
                                           Allocator*      allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(start_token.type == RETURN || start_token.type == BREAK || start_token.type == CONTINUE);

    JumpStatement* jump = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*jump));
    if (!jump) { return ALLOCATION_FAILED; }

    *jump = (JumpStatement){
        .base  = STATEMENT_INIT(JUMP_VTABLE, start_token),
        .value = value,
    };

    *jump_stmt = jump;
    return SUCCESS;
}

void jump_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    JumpStatement* jump = (JumpStatement*)node;
    NODE_VIRTUAL_FREE(jump->value, allocator);

    ALLOCATOR_PTR_FREE(allocator, jump);
}

[[nodiscard]] Status
jump_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
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
    }

    JumpStatement* jump = (JumpStatement*)node;
    if (jump->value) {
        TRY(string_builder_append(sb, ' '));
        TRY(NODE_VIRTUAL_RECONSTRUCT(jump->value, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status
jump_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    const Token start_token = node->start_token;

    JumpStatement* jump           = (JumpStatement*)node;
    SemanticType*  resulting_type = nullptr;
    switch (start_token.type) {
    case RETURN:
    case BREAK:
        if (jump->value) {
            TRY(NODE_VIRTUAL_ANALYZE(jump->value, parent, errors));
            resulting_type = semantic_context_move_analyzed(parent);
        }
        break;
    case CONTINUE:
        assert(!jump->value);
        break;
    default:
        UNREACHABLE;
    }

    parent->analyzed_type = resulting_type;
    return SUCCESS;
}
