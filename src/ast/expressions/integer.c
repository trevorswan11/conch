#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/integer.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS integer_literal_expression_create(Token                      token,
                                             int64_t                    value,
                                             IntegerLiteralExpression** int_expr,
                                             memory_alloc_fn            memory_alloc) {
    assert(memory_alloc);
    assert(token.slice.ptr);
    IntegerLiteralExpression* integer = memory_alloc(sizeof(IntegerLiteralExpression));
    if (!integer) {
        return ALLOCATION_FAILED;
    }

    *integer = (IntegerLiteralExpression){
        .base  = EXPRESSION_INIT(INTEGER_VTABLE),
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

TRY_STATUS
integer_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    IntegerLiteralExpression* integer = (IntegerLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, integer->token.slice));
    return SUCCESS;
}

TRY_STATUS uinteger_literal_expression_create(Token                              token,
                                              uint64_t                           value,
                                              UnsignedIntegerLiteralExpression** int_expr,
                                              memory_alloc_fn                    memory_alloc) {
    assert(memory_alloc);
    UnsignedIntegerLiteralExpression* integer =
        memory_alloc(sizeof(UnsignedIntegerLiteralExpression));
    if (!integer) {
        return ALLOCATION_FAILED;
    }

    *integer = (UnsignedIntegerLiteralExpression){
        .base  = EXPRESSION_INIT(UNSIGNED_INTEGER_VTABLE),
        .token = token,
        .value = value,
    };

    *int_expr = integer;
    return SUCCESS;
}

void uinteger_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    UnsignedIntegerLiteralExpression* integer = (UnsignedIntegerLiteralExpression*)node;
    free_alloc(integer);
}

Slice uinteger_literal_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    UnsignedIntegerLiteralExpression* integer = (UnsignedIntegerLiteralExpression*)node;
    return integer->token.slice;
}

TRY_STATUS
uinteger_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    MAYBE_UNUSED(symbol_map);
    if (!sb) {
        return NULL_PARAMETER;
    }
    ASSERT_NODE(node);

    UnsignedIntegerLiteralExpression* integer = (UnsignedIntegerLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, integer->token.slice));
    return SUCCESS;
}
