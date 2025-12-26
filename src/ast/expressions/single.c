#include <assert.h>

#include "ast/expressions/single.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

#define SINGLE_STMT_CREATE(T, custom_vtab, out_expr)              \
    assert(memory_alloc);                                         \
                                                                  \
    typedef T S;                                                  \
    S*        temp = memory_alloc(sizeof(S));                     \
    if (!temp) { return ALLOCATION_FAILED; }                      \
                                                                  \
    *temp       = (S){EXPRESSION_INIT(custom_vtab, start_token)}; \
    *(out_expr) = temp;                                           \
    return SUCCESS

#define SINGLE_STMT_RECONSTRUCT(string)           \
    ASSERT_EXPRESSION(node);                      \
    MAYBE_UNUSED(node);                           \
    MAYBE_UNUSED(symbol_map);                     \
    assert(sb);                                   \
                                                  \
    TRY(string_builder_append_str_z(sb, string)); \
    return SUCCESS

void single_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);
    free_alloc(node);
}

[[nodiscard]] Status
nil_expression_create(Token start_token, NilExpression** nil_expr, memory_alloc_fn memory_alloc) {
    SINGLE_STMT_CREATE(NilExpression, NIL_VTABLE, nil_expr);
}

[[nodiscard]] Status
nil_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("nil");
}

[[nodiscard]] Status
nil_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_NIL, true, semantic_context_allocator(parent).memory_alloc);
}

[[nodiscard]] Status ignore_expression_create(Token              start_token,
                                              IgnoreExpression** ignore_expr,
                                              memory_alloc_fn    memory_alloc) {
    SINGLE_STMT_CREATE(IgnoreExpression, IGNORE_VTABLE, ignore_expr);
}

[[nodiscard]] Status
ignore_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("_");
}

[[nodiscard]] Status
ignore_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return SUCCESS;
}
