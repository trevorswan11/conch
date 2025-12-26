#ifndef TYPE_EXPR_H
#define TYPE_EXPR_H

#include "lexer/keywords.h"

#include "ast/expressions/expression.h"

#include "util/containers/array_list.h"

static const Keyword ALL_PRIMITIVES[] = {
    KEYWORD_INT,
    KEYWORD_UINT,
    KEYWORD_SIZE,
    KEYWORD_FLOAT,
    KEYWORD_BYTE,
    KEYWORD_STRING,
    KEYWORD_BOOL,
    KEYWORD_VOID,
};

typedef struct IdentifierExpression   IdentifierExpression;
typedef struct TypeExpression         TypeExpression;
typedef struct StructExpression       StructExpression;
typedef struct EnumExpression         EnumExpression;
typedef struct ArrayLiteralExpression ArrayLiteralExpression;

typedef enum {
    EXPLICIT_IDENT,
    EXPLICIT_TYPEOF,
    EXPLICIT_FN,
    EXPLICIT_STRUCT,
    EXPLICIT_ENUM,
    EXPLICIT_ARRAY,
} ExplicitTypeTag;

typedef struct ExplicitIdentType {
    IdentifierExpression* name;
    ArrayList             generics;
} ExplicitIdentType;

typedef struct ExplicitFunctionType {
    ArrayList       generics;
    ArrayList       parameters;
    TypeExpression* return_type;
} ExplicitFunctionType;

typedef struct ExplicitArrayType {
    ArrayList       dimensions;
    TypeExpression* inner_type;
} ExplicitArrayType;

typedef union {
    ExplicitIdentType    ident_type;
    Expression*          referred_type;
    ExplicitFunctionType function_type;
    StructExpression*    struct_type;
    EnumExpression*      enum_type;
    ExplicitArrayType    array_type;
} ExplicitTypeUnion;

typedef struct ExplicitType {
    ExplicitTypeTag   tag;
    ExplicitTypeUnion variant;
    bool              nullable;
    bool              primitive;
} ExplicitType;

typedef struct ImplicitType {
    char _;
} ImplicitType;

typedef enum {
    EXPLICIT,
    IMPLICIT,
} TypeExpressionTag;

typedef union {
    ExplicitType explicit_type;
    ImplicitType implicit_type;
} TypeExpressionUnion;

static const TypeExpressionUnion IMPLICIT_TYPE = {.implicit_type = {'\0'}};

typedef struct TypeExpression {
    Expression          base;
    TypeExpressionTag   tag;
    TypeExpressionUnion variant;
} TypeExpression;

[[nodiscard]] Status type_expression_create(Token               start_token,
                                        TypeExpressionTag   tag,
                                        TypeExpressionUnion variant,
                                        TypeExpression**    type_expr,
                                        memory_alloc_fn     memory_alloc);

void             type_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status type_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb);
[[nodiscard]] Status type_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable TYPE_VTABLE = {
    .base =
        {
            .destroy     = type_expression_destroy,
            .reconstruct = type_expression_reconstruct,
            .analyze     = type_expression_analyze,
        },
};

[[nodiscard]] Status explicit_type_reconstruct(ExplicitType   explicit_type,
                                           const HashMap* symbol_map,
                                           StringBuilder* sb);

#endif
