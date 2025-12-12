#include <assert.h>

#include "ast/expressions/match.h"

#include "semantic/context.h"

#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

void free_match_arm_list(ArrayList* arms, free_alloc_fn free_alloc) {
    assert(arms && arms->item_size == sizeof(MatchArm));
    assert(free_alloc);

    MatchArm arm;
    for (size_t i = 0; i < arms->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(arms, i, &arm));
        NODE_VIRTUAL_FREE(arm.pattern, free_alloc);
        arm.pattern = NULL;

        NODE_VIRTUAL_FREE(arm.dispatch, free_alloc);
        arm.dispatch = NULL;
    }

    array_list_deinit(arms);
}

NODISCARD Status match_expression_create(Token             start_token,
                                         Expression*       expression,
                                         ArrayList         arms,
                                         MatchExpression** match_expr,
                                         memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    assert(arms.item_size == sizeof(MatchArm));
    assert(arms.length > 0);
    assert(expression);

    MatchExpression* match = memory_alloc(sizeof(MatchExpression));
    if (!match) {
        return ALLOCATION_FAILED;
    }

    *match = (MatchExpression){
        .base       = EXPRESSION_INIT(MATCH_VTABLE, start_token),
        .expression = expression,
        .arms       = arms,
    };

    *match_expr = match;
    return SUCCESS;
}

void match_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    MatchExpression* match = (MatchExpression*)node;
    NODE_VIRTUAL_FREE(match->expression, free_alloc);
    free_match_arm_list(&match->arms, free_alloc);

    free_alloc(match);
}

NODISCARD Status match_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "match "));

    MatchExpression* match      = (MatchExpression*)node;
    Node*            match_expr = (Node*)match->expression;
    TRY(match_expr->vtable->reconstruct(match_expr, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, " { "));

    MatchArm arm;
    for (size_t i = 0; i < match->arms.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&match->arms, i, &arm));

        Node* arm_pat = (Node*)arm.pattern;
        TRY(arm_pat->vtable->reconstruct(arm_pat, symbol_map, sb));

        TRY(string_builder_append_str_z(sb, " => "));

        Node* arm_dis = (Node*)arm.dispatch;
        TRY(arm_dis->vtable->reconstruct(arm_dis, symbol_map, sb));

        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append_str_z(sb, "}"));
    return SUCCESS;
}

NODISCARD Status match_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
