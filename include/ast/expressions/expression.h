#pragma once

#include "ast/node.h"

#include "util/status.h"

#define ASSERT_EXPRESSION(expr) \
    assert(expr->vtable);       \
    ASSERT_NODE(((Node*)expr));

#define EXPRESSION_INIT(custom_vtab, token)       \
    (Expression) {                                \
        .base =                                   \
            (Node){                               \
                .vtable      = &custom_vtab.base, \
                .start_token = token,             \
            },                                    \
        .vtable = &custom_vtab,                   \
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
