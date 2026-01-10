#ifndef STRUCT_EXPR_H
#define STRUCT_EXPR_H

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

void free_struct_member_list(ArrayList* members, Allocator* allocator);

[[nodiscard]] Status struct_expression_create(Token              start_token,
                                              ArrayList          generics,
                                              ArrayList          members,
                                              StructExpression** struct_expr,
                                              Allocator*         allocator);

void struct_expression_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
struct_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
struct_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable STRUCT_VTABLE = {
    .base =
        {
            .destroy     = struct_expression_destroy,
            .reconstruct = struct_expression_reconstruct,
            .analyze     = struct_expression_analyze,
        },
};

#endif
