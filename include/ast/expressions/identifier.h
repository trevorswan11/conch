#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct IdentifierExpression {
    Expression base;
    MutSlice   name;
} IdentifierExpression;

NODISCARD Status identifier_expression_create(Token                  start_token,
                                              MutSlice               name,
                                              IdentifierExpression** ident_expr,
                                              memory_alloc_fn        memory_alloc);

void             identifier_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status identifier_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb);

static const ExpressionVTable IDENTIFIER_VTABLE = {
    .base =
        {
            .destroy     = identifier_expression_destroy,
            .reconstruct = identifier_expression_reconstruct,
        },
};
