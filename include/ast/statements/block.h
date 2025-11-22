#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

// Frees all allocated statements and resets the buffer length.
void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc);

// TODO: Maybe use union for walrus vs typed assign
typedef struct {
    Statement base;
    ArrayList statements;
} BlockStatement;

TRY_STATUS block_statement_create(BlockStatement** block_stmt, Allocator allocator);

void       block_statement_destroy(Node* node, free_alloc_fn free_alloc);
Slice      block_statement_token_literal(Node* node);
TRY_STATUS block_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable BLOCK_VTABLE = {
    .base =
        {
            .destroy       = block_statement_destroy,
            .token_literal = block_statement_token_literal,
            .reconstruct   = block_statement_reconstruct,
        },
};

TRY_STATUS block_statement_append(BlockStatement* block_stmt, Statement* stmt);
