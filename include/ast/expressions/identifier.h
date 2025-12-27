#ifndef IDENTIFIER_EXPR_H
#define IDENTIFIER_EXPR_H

#include "ast/expressions/expression.h"

typedef struct IdentifierExpression {
    Expression base;
    MutSlice   name;
} IdentifierExpression;

[[nodiscard]] Status identifier_expression_create(Token                  start_token,
                                                  MutSlice               name,
                                                  IdentifierExpression** ident_expr,
                                                  Allocator*             allocator);

void identifier_expression_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
identifier_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
identifier_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable IDENTIFIER_VTABLE = {
    .base =
        {
            .destroy     = identifier_expression_destroy,
            .reconstruct = identifier_expression_reconstruct,
            .analyze     = identifier_expression_analyze,
        },
};

#endif
