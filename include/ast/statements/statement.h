#ifndef STATEMENT_H
#define STATEMENT_H

#include "ast/node.h"

#define ASSERT_STATEMENT(stmt)          \
    assert(((Statement*)stmt)->vtable); \
    ASSERT_NODE(stmt);

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

#endif
