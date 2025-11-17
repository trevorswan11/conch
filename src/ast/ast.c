#include <assert.h>

#include "ast/ast.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

TRY_STATUS ast_init(AST* ast, Allocator allocator) {
    if (!ast) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(allocator);

    ast->allocator = allocator;
    return array_list_init_allocator(&ast->statements, 64, sizeof(Statement*), allocator);
}

void ast_deinit(AST* ast) {
    if (!ast) {
        return;
    }
    ASSERT_ALLOCATOR(ast->allocator);

    ast_free_statements(ast);
    array_list_deinit(&ast->statements);
}

void ast_free_statements(AST* ast) {
    Statement* stmt;
    for (size_t i = 0; i < ast->statements.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&ast->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);
        Node* node = (Node*)stmt;
        ASSERT_NODE(node);
        node->vtable->destroy(node, ast->allocator.free_alloc);
    }
}

TRY_STATUS ast_reconstruct(AST* ast, StringBuilder* sb) {
    assert(ast && ast->statements.data);
    assert(sb && sb->buffer.data);

    array_list_clear_retaining_capacity(&sb->buffer);

    for (size_t i = 0; i < ast->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&ast->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);
        Node* node = (Node*)stmt;
        ASSERT_NODE(node);
        PROPAGATE_IF_ERROR_DO(node->vtable->reconstruct(node, sb), string_builder_deinit(sb));
    }
    return SUCCESS;
}
