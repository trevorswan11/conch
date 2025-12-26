#ifndef JUMP_STMT_H
#define JUMP_STMT_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

typedef struct JumpStatement {
    Statement   base;
    Expression* value;
} JumpStatement;

[[nodiscard]] Status jump_statement_create(Token           start_token,
                                       Expression*     value,
                                       JumpStatement** jump_stmt,
                                       memory_alloc_fn memory_alloc);

void             jump_statement_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status jump_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);
[[nodiscard]] Status jump_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable JUMP_VTABLE = {
    .base =
        {
            .destroy     = jump_statement_destroy,
            .reconstruct = jump_statement_reconstruct,
            .analyze     = jump_statement_analyze,
        },
};

#endif
