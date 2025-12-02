#pragma once

#include "ast/expressions/identifier.h"
#include "ast/statements/block.h"
#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct {
    Statement             base;
    IdentifierExpression* parent;
    BlockStatement*       implementation;
} ImplStatement;

TRY_STATUS
impl_statement_create(Token                 start_token,
                      IdentifierExpression* parent,
                      BlockStatement*       implementation,
                      ImplStatement**       impl_stmt,
                      memory_alloc_fn       memory_alloc);

void       impl_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS impl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable IMPL_VTABLE = {
    .base =
        {
            .destroy     = impl_statement_destroy,
            .reconstruct = impl_statement_reconstruct,
        },
};
