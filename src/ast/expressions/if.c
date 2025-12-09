#include <assert.h>

#include "ast/expressions/if.h"

NODISCARD Status if_expression_create(Token           start_token,
                                      Expression*     condition,
                                      Statement*      consequence,
                                      Statement*      alternate,
                                      IfExpression**  if_expr,
                                      memory_alloc_fn memory_alloc) {
    assert(condition && consequence);
    assert(memory_alloc);

    IfExpression* if_local = memory_alloc(sizeof(IfExpression));
    if (!if_local) {
        return ALLOCATION_FAILED;
    }

    *if_local = (IfExpression){
        .base        = EXPRESSION_INIT(IF_VTABLE, start_token),
        .condition   = condition,
        .consequence = consequence,
        .alternate   = alternate,
    };

    *if_expr = if_local;
    return SUCCESS;
}

void if_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    IfExpression* if_expr = (IfExpression*)node;
    NODE_VIRTUAL_FREE(if_expr->condition, free_alloc);
    NODE_VIRTUAL_FREE(if_expr->consequence, free_alloc);
    NODE_VIRTUAL_FREE(if_expr->alternate, free_alloc);

    free_alloc(if_expr);
}

NODISCARD Status if_expression_reconstruct(Node*          node,
                                           const HashMap* symbol_map,
                                           StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    IfExpression* if_expr = (IfExpression*)node;
    TRY(string_builder_append_str_z(sb, "if "));

    assert(if_expr->condition);
    TRY(string_builder_append(sb, '('));
    Node* condition_node = (Node*)if_expr->condition;
    TRY(condition_node->vtable->reconstruct(condition_node, symbol_map, sb));
    TRY(string_builder_append(sb, ')'));

    TRY(string_builder_append(sb, ' '));

    assert(if_expr->consequence);
    Node* consequence_node = (Node*)if_expr->consequence;
    TRY(consequence_node->vtable->reconstruct(consequence_node, symbol_map, sb));

    if (if_expr->alternate) {
        TRY(string_builder_append_str_z(sb, " else "));
        Node* alternate_node = (Node*)if_expr->alternate;
        TRY(alternate_node->vtable->reconstruct(alternate_node, symbol_map, sb));
    }

    return SUCCESS;
}
