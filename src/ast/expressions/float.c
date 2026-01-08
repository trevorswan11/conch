#include <assert.h>

#include "ast/expressions/float.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status float_literal_expression_create(Token                    start_token,
                                                     double                   value,
                                                     FloatLiteralExpression** float_expr,
                                                     Allocator*               allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(start_token.slice.ptr);

    FloatLiteralExpression* float_local = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*float_local));
    if (!float_local) { return ALLOCATION_FAILED; }

    *float_local = (FloatLiteralExpression){
        .base  = EXPRESSION_INIT(FLOAT_VTABLE, start_token),
        .value = value,
    };

    *float_expr = float_local;
    return SUCCESS;
}

void float_literal_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    FloatLiteralExpression* float_expr = (FloatLiteralExpression*)node;
    ALLOCATOR_PTR_FREE(allocator, float_expr);
}

[[nodiscard]] Status float_literal_expression_reconstruct(
    Node* node, [[maybe_unused]] const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    return SUCCESS;
}

[[nodiscard]] Status float_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                      SemanticContext*            parent,
                                                      [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_FLOATING_POINT, false, semantic_context_allocator(parent));
}
