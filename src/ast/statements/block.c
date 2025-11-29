#include <assert.h>

#include "ast/ast.h"
#include "ast/statements/block.h"

TRY_STATUS
block_statement_create(Token start_token, BlockStatement** block_stmt, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);

    BlockStatement* block = allocator.memory_alloc(sizeof(BlockStatement));
    if (!block) {
        return ALLOCATION_FAILED;
    }

    ArrayList statements;
    PROPAGATE_IF_ERROR_DO(array_list_init_allocator(&statements, 8, sizeof(Statement*), allocator),
                          allocator.free_alloc(block));

    *block = (BlockStatement){
        .base       = STATEMENT_INIT(BLOCK_VTABLE, start_token),
        .statements = statements,
    };

    *block_stmt = block;
    return SUCCESS;
}

void block_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    BlockStatement* block = (BlockStatement*)node;

    clear_statement_list(&block->statements, free_alloc);
    array_list_deinit(&block->statements);

    free_alloc(block);
}

TRY_STATUS block_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    assert(node && symbol_map && sb);
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "{ "));

    BlockStatement* block = (BlockStatement*)node;
    for (size_t i = 0; i < block->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&block->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);

        Node* stmt_node = (Node*)stmt;
        PROPAGATE_IF_ERROR(stmt_node->vtable->reconstruct(stmt_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " }"));
    return SUCCESS;
}

TRY_STATUS block_statement_append(BlockStatement* block_stmt, Statement* stmt) {
    assert(block_stmt && block_stmt->statements.data && stmt);
    PROPAGATE_IF_ERROR(array_list_push(&block_stmt->statements, &stmt));
    return SUCCESS;
}
