#pragma once

#include "ast/node.h"

#define ASSERT_STATEMENT(stmt) \
    assert(stmt->vtable);      \
    ASSERT_NODE(((Node*)stmt));

#define STATEMENT_INIT(custom_vtab, token)        \
    (Statement) {                                 \
        .base =                                   \
            (Node){                               \
                .vtable      = &custom_vtab.base, \
                .start_token = token,             \
            },                                    \
        .vtable = &custom_vtab,                   \
    }

typedef struct Statement       Statement;
typedef struct StatementVTable StatementVTable;

struct StatementVTable {
    NodeVTable base;
};

struct Statement {
    Node                   base;
    const StatementVTable* vtable;
};
