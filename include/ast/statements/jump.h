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
} JumpStatement;

TRY_STATUS jump_statement_create(Token           start_token,
                                 Expression*     value,
                                 JumpStatement** ret_stmt,
                                 memory_alloc_fn memory_alloc);

void       jump_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS jump_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable JUMP_VTABLE = {
    .base =
        {
            .destroy     = jump_statement_destroy,
            .reconstruct = jump_statement_reconstruct,
        },
};
