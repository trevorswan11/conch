#pragma once

#include "ast/statements/statement.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct BlockStatement       BlockStatement;

typedef struct ImplStatement {
    Statement             base;
    IdentifierExpression* parent;
    BlockStatement*       implementation;
} ImplStatement;

NODISCARD Status impl_statement_create(Token                 start_token,
                                       IdentifierExpression* parent,
                                       BlockStatement*       implementation,
                                       ImplStatement**       impl_stmt,
                                       memory_alloc_fn       memory_alloc);

void             impl_statement_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status impl_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);
NODISCARD Status impl_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable IMPL_VTABLE = {
    .base =
        {
            .destroy     = impl_statement_destroy,
            .reconstruct = impl_statement_reconstruct,
            .analyze     = impl_statement_analyze,
        },
};
