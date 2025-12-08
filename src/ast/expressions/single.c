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
    return SUCCESS

#define SINGLE_STMT_RECONSTRUCT(string)           \
    assert(sb);                                   \
    MAYBE_UNUSED(node);                           \
    MAYBE_UNUSED(symbol_map);                     \
                                                  \
    TRY(string_builder_append_str_z(sb, string)); \
    return SUCCESS

void single_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    free_alloc(node);
}

NODISCARD Status nil_expression_create(Token           start_token,
                                       NilExpression** nil_expr,
                                       memory_alloc_fn memory_alloc) {
    SINGLE_STMT_CREATE(NilExpression, NIL_VTABLE, nil_expr);
}

NODISCARD Status nil_expression_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("nil");
}

NODISCARD Status ignore_expression_create(Token              start_token,
                                          IgnoreExpression** ignore_expr,
                                          memory_alloc_fn    memory_alloc) {
    SINGLE_STMT_CREATE(IgnoreExpression, IGNORE_VTABLE, ignore_expr);
}

NODISCARD Status ignore_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("_");
}
