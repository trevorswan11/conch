#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/float.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS float_literal_expression_create(Token                    token,
                                           double                   value,
                                           FloatLiteralExpression** float_expr,
                                           memory_alloc_fn          memory_alloc) {
    assert(memory_alloc);
    assert(token.slice.ptr);
    FloatLiteralExpression* float_local = memory_alloc(sizeof(FloatLiteralExpression));
    if (!float_local) {
        return ALLOCATION_FAILED;
    }

    *float_local = (FloatLiteralExpression){
        .base  = EXPRESSION_INIT(FLOAT_VTABLE),
        .token = token,
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

Slice float_literal_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    FloatLiteralExpression* float_expr = (FloatLiteralExpression*)node;
    return float_expr->token.slice;
}

TRY_STATUS float_literal_expression_reconstruct(Node* node, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    FloatLiteralExpression* float_expr = (FloatLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, float_expr->token.slice));
    return SUCCESS;
}
