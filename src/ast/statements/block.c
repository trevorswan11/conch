#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/block.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc) {
    assert(statements && statements->data);
    assert(free_alloc);

    Statement* stmt;
    for (size_t i = 0; i < statements->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(statements, i, &stmt));
        ASSERT_STATEMENT(stmt);
        Node* node = (Node*)stmt;
        ASSERT_NODE(node);
        node->vtable->destroy(node, free_alloc);
    }

    array_list_clear_retaining_capacity(statements);
}

TRY_STATUS block_statement_create(BlockStatement** block_stmt, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);

    BlockStatement* block = allocator.memory_alloc(sizeof(BlockStatement));
    if (!block) {
        return ALLOCATION_FAILED;
    }

    ArrayList statements;
    PROPAGATE_IF_ERROR_DO(array_list_init_allocator(&statements, 8, sizeof(Statement*), allocator),
                          allocator.free_alloc(block));

    *block = (BlockStatement){
        .base       = STATEMENT_INIT(BLOCK_VTABLE),
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

Slice block_statement_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return slice_from_str_z(token_type_name(LBRACE));
}

TRY_STATUS block_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    assert(node && symbol_map && sb);
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, "{ ", 2));

    BlockStatement* block = (BlockStatement*)node;
    for (size_t i = 0; i < block->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&block->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);

        Node* stmt_node = (Node*)stmt;
        PROPAGATE_IF_ERROR(stmt_node->vtable->reconstruct(stmt_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append_many(sb, " }", 2));
    return SUCCESS;
}

TRY_STATUS block_statement_append(BlockStatement* block_stmt, Statement* stmt) {
    assert(block_stmt && block_stmt->statements.data && stmt);
    PROPAGATE_IF_ERROR(array_list_push(&block_stmt->statements, &stmt));
    return SUCCESS;
}
