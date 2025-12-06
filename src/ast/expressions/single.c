#include <assert.h>

#include "ast/expressions/single.h"

#define SINGLE_STMT_CREATE(type, custom_vtab, out_expr)            \
    assert(memory_alloc);                                          \
    type* temp = memory_alloc(sizeof(type));                       \
    if (!temp) {                                                   \
        return ALLOCATION_FAILED;                                  \
    }                                                              \
                                                                   \
    *temp     = (type){EXPRESSION_INIT(custom_vtab, start_token)}; \
    *out_expr = temp;                                              \
    return SUCCESS;

#define SINGLE_STMT_RECONSTRUCT(string)                          \
    if (!sb) {                                                   \
        return NULL_PARAMETER;                                   \
    }                                                            \
    MAYBE_UNUSED(node);                                          \
    MAYBE_UNUSED(symbol_map);                                    \
                                                                 \
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, string)); \
    return SUCCESS;

void single_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    free_alloc(node);
}

TRY_STATUS
nil_expression_create(Token start_token, NilExpression** nil_expr, memory_alloc_fn memory_alloc) {
    SINGLE_STMT_CREATE(NilExpression, NIL_VTABLE, nil_expr);
}

TRY_STATUS
nil_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("nil");
}

TRY_STATUS continue_expression_create(Token                start_token,
                                      ContinueExpression** continue_expr,
                                      memory_alloc_fn      memory_alloc) {
    SINGLE_STMT_CREATE(ContinueExpression, CONTINUE_VTABLE, continue_expr);
}

TRY_STATUS
continue_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("continue");
}

TRY_STATUS ignore_expression_create(Token              start_token,
                                    IgnoreExpression** ignore_expr,
                                    memory_alloc_fn    memory_alloc) {
    SINGLE_STMT_CREATE(IgnoreExpression, IGNORE_VTABLE, ignore_expr);
}

TRY_STATUS
ignore_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("_");
}
