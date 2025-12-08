#pragma once

#include <stdint.h>

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct MatchArm {
    Expression* pattern;
    Statement*  dispatch;
} MatchArm;

typedef struct MatchExpression {
    Expression  base;
    Expression* expression;
    ArrayList   arms;
} MatchExpression;

void free_match_arm_list(ArrayList* arms, free_alloc_fn free_alloc);

NODISCARD Status match_expression_create(Token             start_token,
                                         Expression*       expression,
                                         ArrayList         arms,
                                         MatchExpression** match_expr,
                                         memory_alloc_fn   memory_alloc);

void             match_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status match_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb);

static const ExpressionVTable MATCH_VTABLE = {
    .base =
        {
            .destroy     = match_expression_destroy,
            .reconstruct = match_expression_reconstruct,
        },
};
