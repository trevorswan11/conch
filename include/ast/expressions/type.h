#pragma once

#include <stdint.h>

#include "lexer/keywords.h"
#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/node.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

static const Keyword ALL_PRIMITIVES[] = {
    KEYWORD_INT,
    KEYWORD_UINT,
    KEYWORD_FLOAT,
    KEYWORD_BYTE,
    KEYWORD_STRING,
    KEYWORD_BOOL,
    KEYWORD_VOID,
};

typedef struct TypeExpression TypeExpression;

typedef enum {
    EXPLICIT_IDENT,
    EXPLICIT_FN,
} ExplicitTypeTag;

typedef struct {
    ArrayList       fn_type_params;
    TypeExpression* return_type;
} ExplicitFunctionType;

typedef union {
    IdentifierExpression* ident_type_name;
    ExplicitFunctionType  function_type;
} ExplicitTypeUnion;

typedef struct {
    ExplicitTypeTag   tag;
    ExplicitTypeUnion variant;
    bool              nullable;
    bool              primitive;
} ExplicitType;

typedef struct {
    char _;
} ImplicitType;

typedef enum {
    EXPLICIT,
    IMPLICIT,
} TypeTag;

typedef union {
    ExplicitType explicit_type;
    ImplicitType implicit_type;
} TypeUnion;

static const TypeUnion IMPLICIT_TYPE = {.implicit_type = {'\0'}};

typedef struct {
    TypeTag   tag;
    TypeUnion variant;
} Type;

typedef struct TypeExpression {
    Expression base;
    Type       type;
} TypeExpression;

TRY_STATUS
type_expression_create(Token            start_token,
                       TypeTag          tag,
                       TypeUnion        variant,
                       TypeExpression** type_expr,
                       memory_alloc_fn  memory_alloc);

void type_expression_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
type_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const ExpressionVTable TYPE_VTABLE = {
    .base =
        {
            .destroy     = type_expression_destroy,
            .reconstruct = type_expression_reconstruct,
        },
};
