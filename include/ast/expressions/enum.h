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

typedef struct EnumVariant {
    IdentifierExpression* name;
    Expression*           value;
} EnumVariant;

typedef struct EnumExpression {
    Expression base;
    ArrayList  variants;
} EnumExpression;

void free_enum_variant_list(ArrayList* variants, free_alloc_fn free_alloc);

NODISCARD Status enum_expression_create(Token            start_token,
                                        ArrayList        variants,
                                        EnumExpression** enum_expr,
                                        memory_alloc_fn  memory_alloc);

void             enum_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status enum_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb);

static const ExpressionVTable ENUM_VTABLE = {
    .base =
        {
            .destroy     = enum_expression_destroy,
            .reconstruct = enum_expression_reconstruct,
        },
};
