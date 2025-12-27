#include <assert.h>

#include "ast/expressions/if.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status if_expression_create(Token          start_token,
                                          Expression*    condition,
                                          Statement*     consequence,
                                          Statement*     alternate,
                                          IfExpression** if_expr,
                                          Allocator*     allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ASSERT_EXPRESSION(condition);
    ASSERT_EXPRESSION(consequence);

    IfExpression* if_local = ALLOCATOR_PTR_MALLOC(allocator, sizeof(IfExpression));
    if (!if_local) { return ALLOCATION_FAILED; }

    *if_local = (IfExpression){
        .base        = EXPRESSION_INIT(IF_VTABLE, start_token),
        .condition   = condition,
        .consequence = consequence,
        .alternate   = alternate,
    };

    *if_expr = if_local;
    return SUCCESS;
}

void if_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    IfExpression* if_expr = (IfExpression*)node;
    NODE_VIRTUAL_FREE(if_expr->condition, allocator);
    NODE_VIRTUAL_FREE(if_expr->consequence, allocator);
    NODE_VIRTUAL_FREE(if_expr->alternate, allocator);

    ALLOCATOR_PTR_FREE(allocator, if_expr);
}

[[nodiscard]] Status
if_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    IfExpression* if_expr = (IfExpression*)node;

    ASSERT_EXPRESSION(if_expr->condition);
    TRY(string_builder_append_str_z(sb, "if ("));
    TRY(NODE_VIRTUAL_RECONSTRUCT(if_expr->condition, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, ") "));

    ASSERT_EXPRESSION(if_expr->consequence);
    TRY(NODE_VIRTUAL_RECONSTRUCT(if_expr->consequence, symbol_map, sb));

    if (if_expr->alternate) {
        ASSERT_EXPRESSION(if_expr->alternate);
        TRY(string_builder_append_str_z(sb, " else "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(if_expr->alternate, symbol_map, sb));
    }

    return SUCCESS;
}

[[nodiscard]] Status if_expression_analyze(Node*                             node,
                                           [[maybe_unused]] SemanticContext* parent,
                                           [[maybe_unused]] ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    [[maybe_unused]] IfExpression* if_expr = (IfExpression*)node;
    ASSERT_EXPRESSION(if_expr->condition);
    ASSERT_EXPRESSION(if_expr->consequence);

    return NOT_IMPLEMENTED;
}
