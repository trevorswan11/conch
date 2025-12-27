#include <assert.h>

#include "ast/expressions/string.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status string_literal_expression_create(Token                     start_token,
                                                      StringLiteralExpression** string_expr,
                                                      Allocator*                allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    StringLiteralExpression* string =
        ALLOCATOR_PTR_MALLOC(allocator, sizeof(StringLiteralExpression));
    if (!string) { return ALLOCATION_FAILED; }

    MutSlice slice;
    TRY_DO(promote_token_string(start_token, &slice, allocator),
           ALLOCATOR_PTR_FREE(allocator, string));

    *string = (StringLiteralExpression){
        .base  = EXPRESSION_INIT(STRING_VTABLE, start_token),
        .slice = slice,
    };

    *string_expr = string;
    return SUCCESS;
}

void string_literal_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    StringLiteralExpression* string_expr = (StringLiteralExpression*)node;
    if (string_expr->slice.ptr) { ALLOCATOR_PTR_FREE(allocator, string_expr->slice.ptr); }

    ALLOCATOR_PTR_FREE(allocator, string_expr);
}

[[nodiscard]] Status string_literal_expression_reconstruct(
    Node* node, [[maybe_unused]] const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    // The tokenizer drops the start of multiline strings so we have to reconstruct here
    if (node->start_token.type == MULTILINE_STRING) {
        TRY(string_builder_append_str_z(sb, "\\\\"));
    }

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    if (node->start_token.type == MULTILINE_STRING) { TRY(string_builder_append(sb, '\n')); }
    return SUCCESS;
}

[[nodiscard]] Status string_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                       SemanticContext*            parent,
                                                       [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_STR, false, semantic_context_allocator(parent));
}
