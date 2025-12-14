#include <assert.h>

#include "ast/expressions/integer.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

#define INTEGER_EXPR_CREATE(type, custom_vtab, out_expr)    \
    assert(memory_alloc);                                   \
    assert(start_token.slice.ptr);                          \
    type* integer = memory_alloc(sizeof(type));             \
    if (!integer) {                                         \
        return ALLOCATION_FAILED;                           \
    }                                                       \
                                                            \
    *integer = (type){                                      \
        .base  = EXPRESSION_INIT(custom_vtab, start_token), \
        .value = value,                                     \
    };                                                      \
                                                            \
    *out_expr = integer;                                    \
    return SUCCESS

void integer_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    free_alloc(node);
}

NODISCARD Status integer_expression_reconstruct(Node*          node,
                                                const HashMap* symbol_map,
                                                StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    MAYBE_UNUSED(symbol_map);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}

NODISCARD Status integer_literal_expression_create(Token                      start_token,
                                                   int64_t                    value,
                                                   IntegerLiteralExpression** int_expr,
                                                   memory_alloc_fn            memory_alloc) {
    INTEGER_EXPR_CREATE(IntegerLiteralExpression, INTEGER_VTABLE, int_expr);
}

NODISCARD Status integer_literal_expression_analyze(Node*            node,
                                                    SemanticContext* parent,
                                                    ArrayList*       errors) {
    PRIMITIVE_ANALYZE(SIGNED_INTEGER);
}

NODISCARD Status uinteger_literal_expression_create(Token                              start_token,
                                                    uint64_t                           value,
                                                    UnsignedIntegerLiteralExpression** int_expr,
                                                    memory_alloc_fn memory_alloc) {
    INTEGER_EXPR_CREATE(UnsignedIntegerLiteralExpression, UINTEGER_VTABLE, int_expr);
}

NODISCARD Status uinteger_literal_expression_analyze(Node*            node,
                                                     SemanticContext* parent,
                                                     ArrayList*       errors) {
    PRIMITIVE_ANALYZE(UNSIGNED_INTEGER);
}

NODISCARD Status byte_literal_expression_create(Token                   start_token,
                                                uint8_t                 value,
                                                ByteLiteralExpression** byte_expr,
                                                memory_alloc_fn         memory_alloc) {
    INTEGER_EXPR_CREATE(ByteLiteralExpression, BYTE_VTABLE, byte_expr);
}

NODISCARD Status byte_literal_expression_analyze(Node*            node,
                                                 SemanticContext* parent,
                                                 ArrayList*       errors) {
    PRIMITIVE_ANALYZE(BYTE_INTEGER);
}
