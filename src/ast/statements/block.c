#include <assert.h>

#include "ast/ast.h"
#include "ast/statements/block.h"

#include "semantic/context.h"
#include "semantic/symbol.h"

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
    if (!node) {
        return;
    }
    assert(free_alloc);

    BlockStatement* block = (BlockStatement*)node;
    free_statement_list(&block->statements, free_alloc);

    free_alloc(block);
}

NODISCARD Status block_statement_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "{ "));

    BlockStatement*   block = (BlockStatement*)node;
    ArrayListIterator it    = array_list_iterator_init(&block->statements);

    Statement* stmt;
    while (array_list_iterator_has_next(&it, &stmt)) {
        ASSERT_STATEMENT(stmt);
        TRY(NODE_VIRTUAL_RECONSTRUCT(stmt, symbol_map, sb));
    }

    TRY(string_builder_append_str_z(sb, " }"));
    return SUCCESS;
}

NODISCARD Status block_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);
    Allocator allocator = parent->symbol_table->symbols.allocator;

    SemanticContext* child;
    TRY(semantic_context_create(parent, &child, allocator));

    BlockStatement* block = (BlockStatement*)node;

    ArrayListIterator it = array_list_iterator_init(&block->statements);
    Statement*        stmt;
    while (array_list_iterator_has_next(&it, &stmt)) {
        ASSERT_STATEMENT(stmt);
        TRY_DO(NODE_VIRTUAL_ANALYZE(stmt, child, errors),
               semantic_context_destroy(child, allocator.free_alloc));

        // If a type bubbled up we have to release it
        if (child->analyzed_type) {
            RC_RELEASE(child->analyzed_type, allocator.free_alloc);
            child->analyzed_type = NULL;
        }
    }

    semantic_context_destroy(child, allocator.free_alloc);
    return SUCCESS;
}

NODISCARD Status block_statement_append(BlockStatement* block_stmt, Statement* stmt) {
    assert(block_stmt && block_stmt->statements.data && stmt);
    TRY(array_list_push(&block_stmt->statements, &stmt));
    return SUCCESS;
}
