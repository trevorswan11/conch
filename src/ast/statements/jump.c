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

    JumpStatement* jump = ALLOCATOR_PTR_MALLOC(allocator, sizeof(JumpStatement));
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

    const TokenType start_token_type = node->start_token.type;
    if (start_token_type == RETURN) {
        TRY(string_builder_append_str_z(sb, "return"));
    } else if (start_token_type == BREAK) {
        TRY(string_builder_append_str_z(sb, "break"));
    } else if (start_token_type == CONTINUE) {
        TRY(string_builder_append_str_z(sb, "continue"));
    }

    JumpStatement* jump = (JumpStatement*)node;
    if (jump->value) {
        TRY(string_builder_append(sb, ' '));
        TRY(NODE_VIRTUAL_RECONSTRUCT(jump->value, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status jump_statement_analyze(Node*                             node,
                                            [[maybe_unused]] SemanticContext* parent,
                                            [[maybe_unused]] ArrayList*       errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    [[maybe_unused]] JumpStatement* jump = (JumpStatement*)node;

    return NOT_IMPLEMENTED;
}
