#include <assert.h>

#include "ast/expressions/float.h"

TRY_STATUS float_literal_expression_create(Token                    start_token,
                                           double                   value,
                                           FloatLiteralExpression** float_expr,
                                           memory_alloc_fn          memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    FloatLiteralExpression* float_local = memory_alloc(sizeof(FloatLiteralExpression));
    if (!float_local) {
        return ALLOCATION_FAILED;
    }

    *float_local = (FloatLiteralExpression){
        .base  = EXPRESSION_INIT(FLOAT_VTABLE, start_token),
        .value = value,
    };

    *float_expr = float_local;
    return SUCCESS;
}

void float_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    FloatLiteralExpression* float_expr = (FloatLiteralExpression*)node;
    free_alloc(float_expr);
}

TRY_STATUS
float_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}
