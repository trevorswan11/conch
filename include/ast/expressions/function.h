#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lexer/token.h"

#include "parser/parser.h"

#include "ast/expressions/expression.h"
#include "ast/expressions/type.h"
#include "ast/node.h"
#include "ast/statements/block.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    IdentifierExpression* ident;
    TypeExpression*       type;
    Expression*           default_value;
} Parameter;

// Allocates all function parameters and consumes them from the parser.
TRY_STATUS allocate_parameter_list(Parser* p, ArrayList* parameters, bool* contains_default_param);
void       free_parameter_list(ArrayList* parameters);
TRY_STATUS
reconstruct_parameter_list(ArrayList* parameters, const HashMap* symbol_map, StringBuilder* sb);

typedef struct {
    Expression      base;
    ArrayList       parameters;
    BlockStatement* body;
} FunctionExpression;

TRY_STATUS function_expression_create(Token                start_token,
                                      ArrayList            parameters,
                                      BlockStatement*      body,
                                      FunctionExpression** function_expr,
                                      memory_alloc_fn      memory_alloc);

void  function_expression_destroy(Node* node, free_alloc_fn free_alloc);
Slice function_expression_token_literal(Node* node);
TRY_STATUS
function_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable FUNCTION_VTABLE = {
    .base =
        {
            .destroy       = function_expression_destroy,
            .token_literal = function_expression_token_literal,
            .reconstruct   = function_expression_reconstruct,
        },
};
