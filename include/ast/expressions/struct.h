#pragma once

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct TypeExpression       TypeExpression;

typedef struct StructMember {
    IdentifierExpression* name;
    TypeExpression*       type;
    Expression*           default_value;
} StructMember;

typedef struct StructExpression {
    Expression base;
    ArrayList  generics;
    ArrayList  members;
} StructExpression;

void free_struct_member_list(ArrayList* members, free_alloc_fn free_alloc);

NODISCARD Status struct_expression_create(Token              start_token,
                                          ArrayList          generics,
                                          ArrayList          members,
                                          StructExpression** struct_expr,
                                          memory_alloc_fn    memory_alloc);

void             struct_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status struct_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb);
NODISCARD Status struct_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable STRUCT_VTABLE = {
    .base =
        {
            .destroy     = struct_expression_destroy,
            .reconstruct = struct_expression_reconstruct,
            .analyze     = struct_expression_analyze,
        },
};
