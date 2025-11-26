#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/block.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Expression  base;
    Expression* condition;
    Statement*  consequence;
    Statement*  alternate;
} IfExpression;

TRY_STATUS if_expression_create(Token           start_token,
                                Expression*     condition,
                                Statement*      consequence,
                                Statement*      alternate,
                                IfExpression**  if_expr,
                                memory_alloc_fn memory_alloc);

void if_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
if_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable IF_VTABLE = {
    .base =
        {
            .destroy     = if_expression_destroy,
            .reconstruct = if_expression_reconstruct,
        },
};
