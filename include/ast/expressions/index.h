#ifndef INDEX_EXPR_H
#define INDEX_EXPR_H

#include "ast/expressions/expression.h"

typedef struct IndexExpression {
    Expression  base;
    Expression* array;
    Expression* idx;
} IndexExpression;

[[nodiscard]] Status index_expression_create(Token             start_token,
                                             Expression*       array,
                                             Expression*       idx,
                                             IndexExpression** index_expr,
                                             memory_alloc_fn   memory_alloc);

void index_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status
index_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
index_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable INDEX_VTABLE = {
    .base =
        {
            .destroy     = index_expression_destroy,
            .reconstruct = index_expression_reconstruct,
            .analyze     = index_expression_analyze,
        },
};

#endif
