#pragma once

#include "lexer/token.h"

#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

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
    Statement   base;
    ImportTag   tag;
    ImportUnion variant;
} ImportStatement;

NODISCARD Status import_statement_create(Token             start_token,
                                         ImportTag         tag,
                                         ImportUnion       variant,
                                         ImportStatement** import_stmt,
                                         memory_alloc_fn   memory_alloc);

void             import_statement_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status import_statement_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb);

static const StatementVTable IMPORT_VTABLE = {
    .base =
        {
            .destroy     = import_statement_destroy,
            .reconstruct = import_statement_reconstruct,
        },
};
