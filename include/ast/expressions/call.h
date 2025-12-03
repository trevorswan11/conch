#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef Expression* Argument;

typedef struct CallExpression {
    Expression  base;
    Expression* function;
    ArrayList   arguments;
} CallExpression;

TRY_STATUS call_expression_create(Token            start_token,
                                  Expression*      function,
                                  ArrayList        arguments,
                                  CallExpression** call_expr,
                                  memory_alloc_fn  memory_alloc);

void call_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
call_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable CALL_VTABLE = {
    .base =
        {
            .destroy     = call_expression_destroy,
            .reconstruct = call_expression_reconstruct,
        },
};
