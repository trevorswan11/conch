#include "ast/ast.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

bool ast_init(AST* ast) {
    if (!ast) {
        return false;
    }

    return array_list_init(&ast->statements, 64, sizeof(Statement*));
}

void ast_deinit(AST* ast) {
    if (!ast) {
        return;
    }

    Statement* stmt;
    for (size_t i = 0; i < ast->statements.length; i++) {
        array_list_get(&ast->statements, i, &stmt);
        Node* node = (Node*)stmt;
        node->vtable->destroy(node);
    }

    array_list_deinit(&ast->statements);
}
