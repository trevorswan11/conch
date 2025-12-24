#ifndef CALL_EXPR_H
#define CALL_EXPR_H

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

typedef struct CallArgument {
    bool        is_ref;
    Expression* argument;
} CallArgument;

typedef struct CallExpression {
    Expression  base;
    Expression* function;
    ArrayList   arguments;
    ArrayList   generics;
} CallExpression;

void free_call_expression_list(ArrayList* arguments, free_alloc_fn free_alloc);

NODISCARD Status call_expression_create(Token            start_token,
                                        Expression*      function,
                                        ArrayList        arguments,
                                        ArrayList        generics,
                                        CallExpression** call_expr,
                                        memory_alloc_fn  memory_alloc);

void             call_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status call_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb);
NODISCARD Status call_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable CALL_VTABLE = {
    .base =
        {
            .destroy     = call_expression_destroy,
            .reconstruct = call_expression_reconstruct,
            .analyze     = call_expression_analyze,
        },
};

#endif
