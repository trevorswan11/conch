#include <assert.h>

#include "ast/expressions/bool.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status bool_literal_expression_create(Token                   start_token,
                                                    BoolLiteralExpression** bool_expr,
                                                    Allocator*              allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(start_token.type == TRUE || start_token.type == FALSE);

    BoolLiteralExpression* bool_local =
        ALLOCATOR_PTR_MALLOC(allocator, sizeof(BoolLiteralExpression));
    if (!bool_local) { return ALLOCATION_FAILED; }

    *bool_local = (BoolLiteralExpression){
        .base  = EXPRESSION_INIT(BOOL_VTABLE, start_token),
        .value = start_token.type == TRUE,
    };

    *bool_expr = bool_local;
    return SUCCESS;
}

void bool_literal_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    ALLOCATOR_PTR_FREE(allocator, bool_expr);
}

[[nodiscard]] Status bool_literal_expression_reconstruct(Node*                           node,
                                                         [[maybe_unused]] const HashMap* symbol_map,
                                                         StringBuilder*                  sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    TRY(string_builder_append_str_z(sb, bool_expr->value ? "true" : "false"));
    return SUCCESS;
}

[[nodiscard]] Status bool_literal_expression_analyze([[maybe_unused]] Node*      node,
                                                     SemanticContext*            parent,
                                                     [[maybe_unused]] ArrayList* errors) {
    PRIMITIVE_ANALYZE(STYPE_BOOL, false, semantic_context_allocator(parent));
}
