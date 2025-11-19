#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Statement   base;
    Expression* value;
} ReturnStatement;

TRY_STATUS return_statement_create(Expression*       value,
                                   ReturnStatement** ret_stmt,
                                   memory_alloc_fn   memory_alloc);

void       return_statement_destroy(Node* node, free_alloc_fn free_alloc);
Slice      return_statement_token_literal(Node* node);
TRY_STATUS return_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable RET_VTABLE = {
    .base =
        {
            .destroy       = return_statement_destroy,
            .token_literal = return_statement_token_literal,
            .reconstruct   = return_statement_reconstruct,
        },
};
