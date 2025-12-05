#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct DiscardStatement {
    Statement   base;
    Expression* to_discard;
} DiscardStatement;

TRY_STATUS discard_statement_create(Token              start_token,
                                    Expression*        to_discard,
                                    DiscardStatement** discard_stmt,
                                    memory_alloc_fn    memory_alloc);

void discard_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
discard_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable DISCARD_VTABLE = {
    .base =
        {
            .destroy     = discard_statement_destroy,
            .reconstruct = discard_statement_reconstruct,
        },
};
