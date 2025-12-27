#ifndef DECL_STMT_H
#define DECL_STMT_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

typedef struct IdentifierExpression IdentifierExpression;
typedef struct TypeExpression       TypeExpression;

typedef struct DeclStatement {
    Statement             base;
    IdentifierExpression* ident;
    bool                  is_const;
    TypeExpression*       type;
    Expression*           value;
} DeclStatement;

[[nodiscard]] Status decl_statement_create(Token                 start_token,
                                           IdentifierExpression* ident,
                                           TypeExpression*       type,
                                           Expression*           value,
                                           DeclStatement**       decl_stmt,
                                           Allocator*            allocator);

void decl_statement_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status decl_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable DECL_VTABLE = {
    .base =
        {
            .destroy     = decl_statement_destroy,
            .reconstruct = decl_statement_reconstruct,
            .analyze     = decl_statement_analyze,
        },
};

typedef struct TypeDeclStatement {
    Statement             base;
    IdentifierExpression* ident;
    Expression*           value;
    bool                  primitive_alias;
} TypeDeclStatement;

[[nodiscard]] Status type_decl_statement_create(Token                 start_token,
                                                IdentifierExpression* ident,
                                                Expression*           value,
                                                bool                  primitive_alias,
                                                TypeDeclStatement**   type_decl_stmt,
                                                Allocator* allocator);

void type_decl_statement_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
type_decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
type_decl_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable TYPE_DECL_VTABLE = {
    .base =
        {
            .destroy     = type_decl_statement_destroy,
            .reconstruct = type_decl_statement_reconstruct,
            .analyze     = type_decl_statement_analyze,
        },
};

#endif
