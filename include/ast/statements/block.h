#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Statement base;
    ArrayList statements;
} BlockStatement;

TRY_STATUS
block_statement_create(Token start_token, BlockStatement** block_stmt, Allocator allocator);

void       block_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS block_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable BLOCK_VTABLE = {
    .base =
        {
            .destroy     = block_statement_destroy,
            .reconstruct = block_statement_reconstruct,
        },
};

TRY_STATUS block_statement_append(BlockStatement* block_stmt, Statement* stmt);
