#pragma once

#include "ast/node.h"

typedef struct Statement       Statement;
typedef struct StatementVTable StatementVTable;

struct StatementVTable {
    NodeVTable base;
    void (*statement_node)(Statement*);
};

struct Statement {
    Node                   base;
    const StatementVTable* vtable;
};
