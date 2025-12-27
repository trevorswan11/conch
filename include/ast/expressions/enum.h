#ifndef ENUM_EXPR_H
#define ENUM_EXPR_H

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

typedef struct IdentifierExpression IdentifierExpression;

typedef struct EnumVariant {
    IdentifierExpression* name;
    Expression*           value;
} EnumVariant;

typedef struct EnumExpression {
    Expression base;
    ArrayList  variants;
} EnumExpression;

void free_enum_variant_list(ArrayList* variants, Allocator* allocator);

[[nodiscard]] Status enum_expression_create(Token            start_token,
                                            ArrayList        variants,
                                            EnumExpression** enum_expr,
                                            Allocator*       allocator);

void enum_expression_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
enum_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
enum_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable ENUM_VTABLE = {
    .base =
        {
            .destroy     = enum_expression_destroy,
            .reconstruct = enum_expression_reconstruct,
            .analyze     = enum_expression_analyze,
        },
};

#endif
