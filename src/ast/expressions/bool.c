#include <assert.h>

#include "ast/expressions/bool.h"

TRY_STATUS bool_literal_expression_create(Token                   start_token,
                                          BoolLiteralExpression** bool_expr,
                                          memory_alloc_fn         memory_alloc) {
    assert(memory_alloc);
    assert(start_token.type == TRUE || start_token.type == FALSE);
    BoolLiteralExpression* bool_local = memory_alloc(sizeof(BoolLiteralExpression));
    if (!bool_local) {
        return ALLOCATION_FAILED;
    }

    *bool_local = (BoolLiteralExpression){
        .base  = EXPRESSION_INIT(BOOL_VTABLE, start_token),
        .value = start_token.type == TRUE,
    };

    *bool_expr = bool_local;
    return SUCCESS;
}

void bool_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    free_alloc(bool_expr);
}

TRY_STATUS
bool_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, bool_expr->value ? "true" : "false"));
    return SUCCESS;
}
