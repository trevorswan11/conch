#include <assert.h>

#include "ast/expressions/single.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

#define SINGLE_STMT_CREATE(T, custom_vtab, out_expr)              \
    ASSERT_ALLOCATOR_PTR(allocator);                              \
                                                                  \
    typedef T S;                                                  \
    S*        temp = ALLOCATOR_PTR_MALLOC(allocator, sizeof(S));  \
    if (!temp) { return ALLOCATION_FAILED; }                      \
                                                                  \
    *temp       = (S){EXPRESSION_INIT(custom_vtab, start_token)}; \
    *(out_expr) = temp;                                           \
    return SUCCESS

#define SINGLE_STMT_RECONSTRUCT(string)           \
    ASSERT_EXPRESSION(node);                      \
    assert(sb);                                   \
                                                  \
    TRY(string_builder_append_str_z(sb, string)); \
    return SUCCESS

void single_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);
    ALLOCATOR_PTR_FREE(allocator, node);
}

[[nodiscard]] Status
nil_expression_create(Token start_token, NilExpression** nil_expr, Allocator* allocator) {
    SINGLE_STMT_CREATE(NilExpression, NIL_VTABLE, nil_expr);
}

[[nodiscard]] Status nil_expression_reconstruct([[maybe_unused]] Node*          node,
                                                [[maybe_unused]] const HashMap* symbol_map,
                                                StringBuilder*                  sb) {
    SINGLE_STMT_RECONSTRUCT("nil");
}

[[nodiscard]] Status nil_expression_analyze([[maybe_unused]] Node*      node,
                                            SemanticContext*            parent,
                                            [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_NIL, true, semantic_context_allocator(parent));
}

[[nodiscard]] Status
ignore_expression_create(Token start_token, IgnoreExpression** ignore_expr, Allocator* allocator) {
    SINGLE_STMT_CREATE(IgnoreExpression, IGNORE_VTABLE, ignore_expr);
}

[[nodiscard]] Status ignore_expression_reconstruct([[maybe_unused]] Node*          node,
                                                   [[maybe_unused]] const HashMap* symbol_map,
                                                   StringBuilder*                  sb) {
    SINGLE_STMT_RECONSTRUCT("_");
}

[[nodiscard]] Status ignore_expression_analyze([[maybe_unused]] Node*            node,
                                               [[maybe_unused]] SemanticContext* parent,
                                               [[maybe_unused]] ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);
    return SUCCESS;
}
