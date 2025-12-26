#ifndef IMPORT_STMT_H
#define IMPORT_STMT_H

#include "ast/statements/statement.h"

typedef struct IdentifierExpression    IdentifierExpression;
typedef struct StringLiteralExpression StringLiteralExpression;

typedef enum {
    STANDARD,
    USER,
} ImportTag;

typedef union {
    IdentifierExpression*    standard_import;
    StringLiteralExpression* user_import;
} ImportUnion;

typedef struct ImportStatement {
    Statement             base;
    ImportTag             tag;
    ImportUnion           variant;
    IdentifierExpression* alias;
} ImportStatement;

[[nodiscard]] Status import_statement_create(Token                 start_token,
                                         ImportTag             tag,
                                         ImportUnion           variant,
                                         IdentifierExpression* alias,
                                         ImportStatement**     import_stmt,
                                         memory_alloc_fn       memory_alloc);

void             import_statement_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status import_statement_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb);
[[nodiscard]] Status import_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const StatementVTable IMPORT_VTABLE = {
    .base =
        {
            .destroy     = import_statement_destroy,
            .reconstruct = import_statement_reconstruct,
            .analyze     = import_statement_analyze,
        },
};

#endif
