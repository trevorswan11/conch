#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/status.h"

typedef struct JumpStatement {
    Statement   base;
    Expression* value;
} JumpStatement;

NODISCARD Status jump_statement_create(Token           start_token,
                                       Expression*     value,
                                       JumpStatement** ret_stmt,
                                       memory_alloc_fn memory_alloc);

void             jump_statement_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status jump_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);
NODISCARD Status jump_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable JUMP_VTABLE = {
    .base =
        {
            .destroy     = jump_statement_destroy,
            .reconstruct = jump_statement_reconstruct,
            .analyze     = jump_statement_analyze,
        },
};
