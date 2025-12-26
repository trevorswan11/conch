#include <assert.h>

#include "ast/expressions/integer.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

#define INTEGER_EXPR_CREATE(T, custom_vtab, out_expr)       \
    assert(memory_alloc);                                   \
    assert(start_token.slice.ptr);                          \
                                                            \
    typedef T I;                                            \
    I*        integer = memory_alloc(sizeof(I));            \
    if (!integer) { return ALLOCATION_FAILED; }             \
                                                            \
    *integer = (I){                                         \
        .base  = EXPRESSION_INIT(custom_vtab, start_token), \
        .value = value,                                     \
    };                                                      \
                                                            \
    *(out_expr) = integer;                                  \
    return SUCCESS

void integer_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);
    free_alloc(node);
}

[[nodiscard]] Status integer_expression_reconstruct(Node*          node,
                                                const HashMap* symbol_map,
                                                StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    MAYBE_UNUSED(symbol_map);
    assert(sb);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}

[[nodiscard]] Status integer_literal_expression_create(Token                      start_token,
                                                   int64_t                    value,
                                                   IntegerLiteralExpression** int_expr,
                                                   memory_alloc_fn            memory_alloc) {
    INTEGER_EXPR_CREATE(IntegerLiteralExpression, INTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status integer_literal_expression_analyze(Node*            node,
                                                    SemanticContext* parent,
                                                    ArrayList*       errors) {
    PRIMITIVE_ANALYZE(STYPE_SIGNED_INTEGER, false, semantic_context_allocator(parent).memory_alloc);
}

[[nodiscard]] Status uinteger_literal_expression_create(Token                              start_token,
                                                    uint64_t                           value,
                                                    UnsignedIntegerLiteralExpression** int_expr,
                                                    memory_alloc_fn memory_alloc) {
    INTEGER_EXPR_CREATE(UnsignedIntegerLiteralExpression, UINTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status uinteger_literal_expression_analyze(Node*            node,
                                                     SemanticContext* parent,
                                                     ArrayList*       errors) {
    PRIMITIVE_ANALYZE(
        STYPE_UNSIGNED_INTEGER, false, semantic_context_allocator(parent).memory_alloc);
}

[[nodiscard]] Status uzinteger_literal_expression_create(Token                          start_token,
                                                     size_t                         value,
                                                     SizeIntegerLiteralExpression** int_expr,
                                                     memory_alloc_fn                memory_alloc) {
    INTEGER_EXPR_CREATE(SizeIntegerLiteralExpression, UZINTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status uzinteger_literal_expression_analyze(Node*            node,
                                                      SemanticContext* parent,
                                                      ArrayList*       errors) {
    PRIMITIVE_ANALYZE(STYPE_SIZE_INTEGER, false, semantic_context_allocator(parent).memory_alloc);
}

[[nodiscard]] Status byte_literal_expression_create(Token                   start_token,
                                                uint8_t                 value,
                                                ByteLiteralExpression** byte_expr,
                                                memory_alloc_fn         memory_alloc) {
    INTEGER_EXPR_CREATE(ByteLiteralExpression, BYTE_VTABLE, byte_expr);
}

[[nodiscard]] Status byte_literal_expression_analyze(Node*            node,
                                                 SemanticContext* parent,
                                                 ArrayList*       errors) {
    PRIMITIVE_ANALYZE(STYPE_BYTE_INTEGER, false, semantic_context_allocator(parent).memory_alloc);
}
