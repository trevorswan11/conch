#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/status.h"

typedef struct DiscardStatement {
    Statement   base;
    Expression* to_discard;
} DiscardStatement;

NODISCARD Status discard_statement_create(Token              start_token,
                                          Expression*        to_discard,
                                          DiscardStatement** discard_stmt,
                                          memory_alloc_fn    memory_alloc);

void             discard_statement_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status discard_statement_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);
NODISCARD Status discard_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable DISCARD_VTABLE = {
    .base =
        {
            .destroy     = discard_statement_destroy,
            .reconstruct = discard_statement_reconstruct,
            .analyze     = discard_statement_analyze,
        },
};
