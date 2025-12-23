#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ast/node.h"

#define ASSERT_EXPRESSION(expr)          \
    assert(((Expression*)expr)->vtable); \
    ASSERT_NODE(expr);

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

typedef struct ExpressionVTable {
    NodeVTable base;
} ExpressionVTable;

typedef struct Expression {
    Node                    base;
    const ExpressionVTable* vtable;
} Expression;

#endif
