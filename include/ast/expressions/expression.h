#pragma once

#include "ast/node.h"

#include "util/status.h"

#define ASSERT_EXPRESSION(expr) \
    assert(expr->vtable);       \
    assert(expr->base.vtable);

#define EXPRESSION_INIT(custom_vtab)         \
    (Expression) {                           \
        .base =                              \
            (Node){                          \
                .vtable = &custom_vtab.base, \
            },                               \
        .vtable = &custom_vtab,              \
    }

typedef struct Expression       Expression;
typedef struct ExpressionVTable ExpressionVTable;

struct ExpressionVTable {
    NodeVTable base;
};

struct Expression {
    Node                    base;
    const ExpressionVTable* vtable;
};
