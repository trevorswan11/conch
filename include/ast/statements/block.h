#ifndef BLOCK_STMT_H
#define BLOCK_STMT_H

#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

typedef struct BlockStatement {
    Statement base;
    ArrayList statements;
} BlockStatement;

NODISCARD Status block_statement_create(Token            start_token,
                                        BlockStatement** block_stmt,
                                        Allocator        allocator);

void             block_statement_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status block_statement_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb);
NODISCARD Status block_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable BLOCK_VTABLE = {
    .base =
        {
            .destroy     = block_statement_destroy,
            .reconstruct = block_statement_reconstruct,
            .analyze     = block_statement_analyze,
        },
};

NODISCARD Status block_statement_append(BlockStatement* block_stmt, Statement* stmt);

#endif
