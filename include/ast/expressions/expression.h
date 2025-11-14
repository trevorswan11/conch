#pragma once

#include "ast/node.h"

#include "util/status.h"

#define ASSERT_EXPRESSION(expr) \
    assert(expr->vtable);       \
    assert(expr->base.vtable);

typedef struct Expression       Expression;
typedef struct ExpressionVTable ExpressionVTable;

struct ExpressionVTable {
    NodeVTable base;
};

struct Expression {
    Node                    base;
    const ExpressionVTable* vtable;
};
