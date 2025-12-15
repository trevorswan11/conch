#pragma once

#include "ast/expressions/expression.h"

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
NODISCARD Status identifier_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors);

static const ExpressionVTable IDENTIFIER_VTABLE = {
    .base =
        {
            .destroy     = identifier_expression_destroy,
            .reconstruct = identifier_expression_reconstruct,
            .analyze     = identifier_expression_analyze,
        },
};
