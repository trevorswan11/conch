#include <assert.h>

#include "ast/ast.h"
#include "ast/statements/block.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

NODISCARD Status block_statement_create(Token            start_token,
                                        BlockStatement** block_stmt,
                                        Allocator        allocator) {
    ASSERT_ALLOCATOR(allocator);

    BlockStatement* block = allocator.memory_alloc(sizeof(BlockStatement));
    if (!block) {
        return ALLOCATION_FAILED;
    }

    ArrayList statements;
    TRY_DO(array_list_init_allocator(&statements, 8, sizeof(Statement*), allocator),
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
    free_statement_list(&block->statements, free_alloc);

    free_alloc(block);
}

NODISCARD Status block_statement_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "{ "));

    BlockStatement* block = (BlockStatement*)node;
    for (size_t i = 0; i < block->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&block->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);

        Node* stmt_node = (Node*)stmt;
        TRY(stmt_node->vtable->reconstruct(stmt_node, symbol_map, sb));
    }

    TRY(string_builder_append_str_z(sb, " }"));
    return SUCCESS;
}

NODISCARD Status block_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    Allocator allocator = parent->symbol_table->symbols.allocator;

    SemanticContext* child;
    TRY(semantic_context_create(parent, &child, allocator));

    BlockStatement* block = (BlockStatement*)node;
    for (size_t i = 0; i < block->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&block->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);

        TRY_DO(NODE_VIRTUAL_ANALYZE(stmt, child, errors),
               semantic_context_destroy(child, allocator.free_alloc));
        semantic_type_deinit(&child->analyzed_type, allocator.free_alloc);
    }

    semantic_context_destroy(child, allocator.free_alloc);
    return SUCCESS;
}

NODISCARD Status block_statement_append(BlockStatement* block_stmt, Statement* stmt) {
    assert(block_stmt && block_stmt->statements.data && stmt);
    TRY(array_list_push(&block_stmt->statements, &stmt));
    return SUCCESS;
}
