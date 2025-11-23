#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/if.h"
#include "ast/statements/block.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS if_expression_create(Expression*     condition,
                                Statement*      consequence,
                                Statement*      alternate,
                                IfExpression**  if_expr,
                                memory_alloc_fn memory_alloc) {
    if (!condition || !consequence) {
        return NULL_PARAMETER;
    }

    assert(memory_alloc);
    IfExpression* if_local = memory_alloc(sizeof(IfExpression));
    if (!if_local) {
        return ALLOCATION_FAILED;
    }

    *if_local = (IfExpression){
        .base        = EXPRESSION_INIT(IF_VTABLE),
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

    if (if_expr->condition) {
        Node* condition = (Node*)if_expr->condition;
        condition->vtable->destroy(condition, free_alloc);
        condition = NULL;
    }

    if (if_expr->consequence) {
        Node* consequence = (Node*)if_expr->consequence;
        consequence->vtable->destroy(consequence, free_alloc);
        consequence = NULL;
    }

    if (if_expr->alternate) {
        Node* alternate = (Node*)if_expr->alternate;
        alternate->vtable->destroy(alternate, free_alloc);
        alternate = NULL;
    }

    free_alloc(if_expr);
}

Slice if_expression_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return slice_from_str_z(token_type_name(IF));
}

TRY_STATUS
if_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    IfExpression* if_expr = (IfExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, "if ", 3));

    Node* condition_node = (Node*)if_expr->condition;
    PROPAGATE_IF_ERROR(condition_node->vtable->reconstruct(condition_node, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));

    Node* consequence_node = (Node*)if_expr->consequence;
    PROPAGATE_IF_ERROR(consequence_node->vtable->reconstruct(consequence_node, symbol_map, sb));

    if (if_expr->alternate) {
        PROPAGATE_IF_ERROR(string_builder_append_many(sb, " else ", 6));
        Node* alternate_node = (Node*)if_expr->alternate;
        PROPAGATE_IF_ERROR(alternate_node->vtable->reconstruct(alternate_node, symbol_map, sb));
    }

    return SUCCESS;
}
