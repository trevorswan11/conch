#pragma once

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

// TODO: Maybe use union for walrus vs typed assign
typedef struct {
    Statement             base;
    IdentifierExpression* ident;
    bool                  is_const;
    TypeExpression*       type;
    Expression*           value;
} DeclStatement;

TRY_STATUS decl_statement_create(Token                 start_token,
                                 IdentifierExpression* ident,
                                 TypeExpression*       type,
                                 Expression*           value,
                                 DeclStatement**       decl_stmt,
                                 memory_alloc_fn       memory_alloc);

void       decl_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable DECL_VTABLE = {
    .base =
        {
            .destroy     = decl_statement_destroy,
            .reconstruct = decl_statement_reconstruct,
        },
};

typedef struct {
    Statement             base;
    IdentifierExpression* ident;
    Expression*           value;
} TypeDeclStatement;

TRY_STATUS type_decl_statement_create(Token                 start_token,
                                      IdentifierExpression* ident,
                                      Expression*           value,
                                      TypeDeclStatement**   type_decl_stmt,
                                      memory_alloc_fn       memory_alloc);

void type_decl_statement_destroy(Node* node, free_alloc_fn free_alloc);
TRY_STATUS
type_decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);

static const StatementVTable TYPE_DECL_VTABLE = {
    .base =
        {
            .destroy     = type_decl_statement_destroy,
            .reconstruct = type_decl_statement_reconstruct,
        },
};
