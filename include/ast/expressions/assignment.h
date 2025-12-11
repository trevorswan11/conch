#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct AssignmentExpression {
    Expression  base;
    Expression* lhs;
    Expression* rhs;
} AssignmentExpression;

NODISCARD Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
                                              Expression*            rhs,
                                              AssignmentExpression** assignment_expr,
                                              memory_alloc_fn        memory_alloc);

void             assignment_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status assignment_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb);

static const ExpressionVTable ASSIGNMENT_VTABLE = {
    .base =
        {
            .destroy     = assignment_expression_destroy,
            .reconstruct = assignment_expression_reconstruct,
        },
};

typedef struct CompoundAssignmentExpression {
    Expression  base;
    Expression* lhs;
    TokenType   op;
    Expression* rhs;
} CompoundAssignmentExpression;

NODISCARD Status compound_assignment_expression_create(Token                          start_token,
                                                       Expression*                    lhs,
                                                       TokenType                      op,
                                                       Expression*                    rhs,
                                                       CompoundAssignmentExpression** compound_expr,
                                                       memory_alloc_fn                memory_alloc);

void             compound_assignment_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status compound_assignment_expression_reconstruct(Node*          node,
                                                            const HashMap* symbol_map,
                                                            StringBuilder* sb);

static const ExpressionVTable COMPOUND_ASSIGNMENT_VTABLE = {
    .base =
        {
            .destroy     = compound_assignment_expression_destroy,
            .reconstruct = compound_assignment_expression_reconstruct,
        },
};
