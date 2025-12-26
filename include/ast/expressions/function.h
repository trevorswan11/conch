#ifndef FUNCTION_EXPR_H
#define FUNCTION_EXPR_H

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct TypeExpression       TypeExpression;
typedef struct BlockStatement       BlockStatement;

typedef struct Parameter {
    bool                  is_ref;
    IdentifierExpression* ident;
    TypeExpression*       type;
    Expression*           default_value;
} Parameter;

void             free_parameter_list(ArrayList* parameters, free_alloc_fn free_alloc);
[[nodiscard]] Status reconstruct_parameter_list(ArrayList*     parameters,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);

typedef struct FunctionExpression {
    Expression      base;
    ArrayList       generics;
    ArrayList       parameters;
    TypeExpression* return_type;
    BlockStatement* body;
} FunctionExpression;

[[nodiscard]] Status function_expression_create(Token                start_token,
                                            ArrayList            generics,
                                            ArrayList            parameters,
                                            TypeExpression*      return_type,
                                            BlockStatement*      body,
                                            FunctionExpression** function_expr,
                                            memory_alloc_fn      memory_alloc);

void             function_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status function_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb);
[[nodiscard]] Status function_expression_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors);

static const ExpressionVTable FUNCTION_VTABLE = {
    .base =
        {
            .destroy     = function_expression_destroy,
            .reconstruct = function_expression_reconstruct,
            .analyze     = function_expression_analyze,
        },
};

#endif
