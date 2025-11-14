#pragma once

#include "ast/node.h"

#include "util/status.h"

#define ASSERT_STATEMENT(stmt) \
    assert(stmt->vtable);      \
    assert(stmt->base.vtable);

typedef struct Statement       Statement;
typedef struct StatementVTable StatementVTable;

struct StatementVTable {
    NodeVTable base;
};

struct Statement {
    Node                   base;
    const StatementVTable* vtable;
};
