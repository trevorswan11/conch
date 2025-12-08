#pragma once

#include <stdbool.h>

#include "lexer/token.h"

#include "parser/parser.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct TypeExpression       TypeExpression;
typedef struct BlockStatement       BlockStatement;

typedef struct Parameter {
    bool                  is_ref;
    IdentifierExpression* ident;
    TypeExpression*       type;
    Expression*           default_value;
} Parameter;

// Allocates all function parameters and consumes them from the parser.
NODISCARD Status allocate_parameter_list(Parser*    p,
                                         ArrayList* parameters,
                                         bool*      contains_default_param);
void             free_parameter_list(ArrayList* parameters, free_alloc_fn free_alloc);
NODISCARD Status reconstruct_parameter_list(ArrayList*     parameters,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb);

typedef struct FunctionExpression {
    Expression      base;
    ArrayList       generics;
    ArrayList       parameters;
    TypeExpression* return_type;
    BlockStatement* body;
} FunctionExpression;

NODISCARD Status function_expression_create(Token                start_token,
                                            ArrayList            generics,
                                            ArrayList            parameters,
                                            TypeExpression*      return_type,
                                            BlockStatement*      body,
                                            FunctionExpression** function_expr,
                                            memory_alloc_fn      memory_alloc);

void             function_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status function_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb);

static const ExpressionVTable FUNCTION_VTABLE = {
    .base =
        {
            .destroy     = function_expression_destroy,
            .reconstruct = function_expression_reconstruct,
        },
};
