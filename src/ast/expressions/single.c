#include <assert.h>

#include "ast/expressions/single.h"

TRY_STATUS
nil_expression_create(Token start_token, NilExpression** nil_expr, memory_alloc_fn memory_alloc) {
    assert(memory_alloc);
    NilExpression* nil = memory_alloc(sizeof(NilExpression));
    if (!nil) {
        return ALLOCATION_FAILED;
    }

    *nil      = (NilExpression){EXPRESSION_INIT(NIL_VTABLE, start_token)};
    *nil_expr = nil;
    return SUCCESS;
}

void nil_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    NilExpression* nil = (NilExpression*)node;
    free_alloc(nil);
}

TRY_STATUS
nil_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(symbol_map);

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "nil"));
    return SUCCESS;
}

TRY_STATUS continue_expression_create(Token                start_token,
                                      ContinueExpression** continue_expr,
                                      memory_alloc_fn      memory_alloc) {
    assert(memory_alloc);
    ContinueExpression* continue_local = memory_alloc(sizeof(ContinueExpression));
    if (!continue_local) {
        return ALLOCATION_FAILED;
    }

    *continue_local = (ContinueExpression){EXPRESSION_INIT(CONTINUE_VTABLE, start_token)};
    *continue_expr  = continue_local;
    return SUCCESS;
}

void continue_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ContinueExpression* continue_expr = (ContinueExpression*)node;
    free_alloc(continue_expr);
}

TRY_STATUS
continue_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(symbol_map);

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "continue"));
    return SUCCESS;
}
