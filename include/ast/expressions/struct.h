#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct TypeExpression       TypeExpression;

typedef struct StructMember {
    IdentifierExpression* name;
    TypeExpression*       type;
    Expression*           default_value;
} StructMember;

typedef struct StructExpression {
    Expression base;
    ArrayList  members;
} StructExpression;

void free_struct_member_list(ArrayList* members, free_alloc_fn free_alloc);

TRY_STATUS struct_expression_create(Token              start_token,
                                    ArrayList          members,
                                    StructExpression** struct_expr,
                                    memory_alloc_fn    memory_alloc);

void struct_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
struct_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable STRUCT_VTABLE = {
    .base =
        {
            .destroy     = struct_expression_destroy,
            .reconstruct = struct_expression_reconstruct,
        },
};
