#pragma once

#include "ast/node.h"

typedef struct Expression       Expression;
typedef struct ExpressionVTable ExpressionVTable;

struct ExpressionVTable {
    NodeVTable base;
    void (*expression_node)(Expression*);
};

struct Expression {
    Node                    base;
    const ExpressionVTable* vtable;
};
