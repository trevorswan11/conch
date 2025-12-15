#pragma once

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

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
NODISCARD Status match_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable MATCH_VTABLE = {
    .base =
        {
            .destroy     = match_expression_destroy,
            .reconstruct = match_expression_reconstruct,
            .analyze     = match_expression_analyze,
        },
};
