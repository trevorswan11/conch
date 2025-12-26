#ifndef DISCARD_STMT_H
#define DISCARD_STMT_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

typedef struct DiscardStatement {
    Statement   base;
    Expression* to_discard;
} DiscardStatement;

[[nodiscard]] Status discard_statement_create(Token              start_token,
                                          Expression*        to_discard,
                                          DiscardStatement** discard_stmt,
                                          memory_alloc_fn    memory_alloc);

void             discard_statement_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status discard_statement_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);
[[nodiscard]] Status discard_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable DISCARD_VTABLE = {
    .base =
        {
            .destroy     = discard_statement_destroy,
            .reconstruct = discard_statement_reconstruct,
            .analyze     = discard_statement_analyze,
        },
};

#endif
