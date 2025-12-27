#include <assert.h>

#include "ast/expressions/integer.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

#define INTEGER_EXPR_CREATE(T, custom_vtab, out_expr)               \
    ASSERT_ALLOCATOR_PTR(allocator);                                \
    assert(start_token.slice.ptr);                                  \
                                                                    \
    typedef T I;                                                    \
    I*        integer = ALLOCATOR_PTR_MALLOC(allocator, sizeof(I)); \
    if (!integer) { return ALLOCATION_FAILED; }                     \
                                                                    \
    *integer = (I){                                                 \
        .base  = EXPRESSION_INIT(custom_vtab, start_token),         \
        .value = value,                                             \
    };                                                              \
                                                                    \
    *(out_expr) = integer;                                          \
    return SUCCESS

void integer_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);
    ALLOCATOR_PTR_FREE(allocator, node);
}

[[nodiscard]] Status integer_expression_reconstruct(Node*                           node,
                                                    [[maybe_unused]] const HashMap* symbol_map,
                                                    StringBuilder*                  sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}

[[nodiscard]] Status integer_literal_expression_create(Token                      start_token,
                                                       int64_t                    value,
                                                       IntegerLiteralExpression** int_expr,
                                                       Allocator*                 allocator) {
    INTEGER_EXPR_CREATE(IntegerLiteralExpression, INTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status integer_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                        SemanticContext*            parent,
                                                        [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_SIGNED_INTEGER, false, semantic_context_allocator(parent));
}

[[nodiscard]] Status uinteger_literal_expression_create(Token    start_token,
                                                        uint64_t value,
                                                        UnsignedIntegerLiteralExpression** int_expr,
                                                        Allocator* allocator) {
    INTEGER_EXPR_CREATE(UnsignedIntegerLiteralExpression, UINTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status uinteger_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                         SemanticContext*            parent,
                                                         [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_UNSIGNED_INTEGER, false, semantic_context_allocator(parent));
}

[[nodiscard]] Status uzinteger_literal_expression_create(Token                          start_token,
                                                         size_t                         value,
                                                         SizeIntegerLiteralExpression** int_expr,
                                                         Allocator*                     allocator) {
    INTEGER_EXPR_CREATE(SizeIntegerLiteralExpression, UZINTEGER_VTABLE, int_expr);
}

[[nodiscard]] Status uzinteger_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                          SemanticContext*            parent,
                                                          [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_SIZE_INTEGER, false, semantic_context_allocator(parent));
}

[[nodiscard]] Status byte_literal_expression_create(Token                   start_token,
                                                    uint8_t                 value,
                                                    ByteLiteralExpression** byte_expr,
                                                    Allocator*              allocator) {
    INTEGER_EXPR_CREATE(ByteLiteralExpression, BYTE_VTABLE, byte_expr);
}

[[nodiscard]] Status byte_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                     SemanticContext*            parent,
                                                     [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_BYTE_INTEGER, false, semantic_context_allocator(parent));
}
