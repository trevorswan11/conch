#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/integer.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS integer_literal_expression_create(Token                      token,
                                             int64_t                    value,
                                             IntegerLiteralExpression** int_expr,
                                             memory_alloc_fn            memory_alloc) {
    assert(memory_alloc);
    IntegerLiteralExpression* integer = memory_alloc(sizeof(IntegerLiteralExpression));
    if (!integer) {
        return ALLOCATION_FAILED;
    }

    *integer = (IntegerLiteralExpression){
        .base =
            (Expression){
                .base =
                    (Node){
                        .vtable = &INTEGER_VTABLE.base,
                    },
                .vtable = &INTEGER_VTABLE,
            },
        .token = token,
        .value = value,
    };

    *int_expr = integer;
    return SUCCESS;
}

void integer_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    IntegerLiteralExpression* integer = (IntegerLiteralExpression*)node;
    free_alloc(integer);
}

Slice integer_literal_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    IntegerLiteralExpression* integer = (IntegerLiteralExpression*)node;
    return integer->token.slice;
}

TRY_STATUS integer_literal_expression_reconstruct(Node* node, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    IntegerLiteralExpression* integer = (IntegerLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, integer->token.slice));
    return SUCCESS;
}
