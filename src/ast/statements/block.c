#include <assert.h>

#include "ast/ast.h"
#include "ast/statements/block.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status
block_statement_create(Token start_token, BlockStatement** block_stmt, Allocator* allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);

    BlockStatement* block = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*block));
    if (!block) { return ALLOCATION_FAILED; }

    ArrayList statements;
    TRY_DO(array_list_init_allocator(&statements, 8, sizeof(Statement*), allocator),
           ALLOCATOR_PTR_FREE(allocator, block));

    *block = (BlockStatement){
        .base       = STATEMENT_INIT(BLOCK_VTABLE, start_token),
        .statements = statements,
    };

    *block_stmt = block;
    return SUCCESS;
}

void block_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    BlockStatement* block = (BlockStatement*)node;
    free_statement_list(&block->statements, allocator);

    ALLOCATOR_PTR_FREE(allocator, block);
}

[[nodiscard]] Status
block_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "{ "));

    BlockStatement*        block = (BlockStatement*)node;
    ArrayListConstIterator it    = array_list_const_iterator_init(&block->statements);

    Statement* stmt;
    while (array_list_const_iterator_has_next(&it, (void*)&stmt)) {
        ASSERT_STATEMENT(stmt);
        TRY(NODE_VIRTUAL_RECONSTRUCT(stmt, symbol_map, sb));
    }

    TRY(string_builder_append_str_z(sb, " }"));
    return SUCCESS;
}

[[nodiscard]] Status
block_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);
    Allocator* allocator = semantic_context_allocator(parent);

    SemanticContext* child;
    TRY(semantic_context_create(parent, &child, allocator));

    BlockStatement* block = (BlockStatement*)node;

    ArrayListConstIterator it = array_list_const_iterator_init(&block->statements);
    Statement*             stmt;
    while (array_list_const_iterator_has_next(&it, (void*)&stmt)) {
        ASSERT_STATEMENT(stmt);
        TRY_DO(NODE_VIRTUAL_ANALYZE(stmt, child, errors),
               semantic_context_destroy(child, allocator));

        // If a type bubbled up we have to release it
        if (child->analyzed_type) {
            RC_RELEASE(child->analyzed_type, allocator);
            child->analyzed_type = nullptr;
        }
    }

    semantic_context_destroy(child, allocator);
    return SUCCESS;
}

[[nodiscard]] Status block_statement_append(BlockStatement* block_stmt, const Statement* stmt) {
    assert(block_stmt && block_stmt->statements.data && stmt);
    TRY(array_list_push(&block_stmt->statements, (const void*)&stmt));
    return SUCCESS;
}
