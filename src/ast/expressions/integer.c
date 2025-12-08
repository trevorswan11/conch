#include <assert.h>

#include "ast/expressions/integer.h"

NODISCARD Status integer_literal_expression_create(Token                      start_token,
                                                   int64_t                    value,
                                                   IntegerLiteralExpression** int_expr,
                                                   memory_alloc_fn            memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    IntegerLiteralExpression* integer = memory_alloc(sizeof(IntegerLiteralExpression));
    if (!integer) {
        return ALLOCATION_FAILED;
    }

    *integer = (IntegerLiteralExpression){
        .base  = EXPRESSION_INIT(INTEGER_VTABLE, start_token),
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

NODISCARD Status integer_literal_expression_reconstruct(Node*          node,
                                                        const HashMap* symbol_map,
                                                        StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    MAYBE_UNUSED(symbol_map);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}

NODISCARD Status uinteger_literal_expression_create(Token                              start_token,
                                                    uint64_t                           value,
                                                    UnsignedIntegerLiteralExpression** int_expr,
                                                    memory_alloc_fn memory_alloc) {
    assert(memory_alloc);
    UnsignedIntegerLiteralExpression* integer =
        memory_alloc(sizeof(UnsignedIntegerLiteralExpression));
    if (!integer) {
        return ALLOCATION_FAILED;
    }

    *integer = (UnsignedIntegerLiteralExpression){
        .base  = EXPRESSION_INIT(UNSIGNED_INTEGER_VTABLE, start_token),
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
