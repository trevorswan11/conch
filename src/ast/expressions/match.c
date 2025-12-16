#include <assert.h>

#include "ast/expressions/match.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_match_arm_list(ArrayList* arms, free_alloc_fn free_alloc) {
    assert(arms && arms->item_size == sizeof(MatchArm));
    assert(free_alloc);

    ArrayListIterator it = array_list_iterator_init(arms);
    MatchArm          arm;
    while (array_list_iterator_has_next(&it, &arm)) {
        NODE_VIRTUAL_FREE(arm.pattern, free_alloc);
        NODE_VIRTUAL_FREE(arm.dispatch, free_alloc);
    }

    array_list_deinit(arms);
}

NODISCARD Status match_expression_create(Token             start_token,
                                         Expression*       expression,
                                         ArrayList         arms,
                                         Statement*        catch_all,
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
        .catch_all  = catch_all,
    };

    *match_expr = match;
    return SUCCESS;
}

void match_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    MatchExpression* match = (MatchExpression*)node;
    NODE_VIRTUAL_FREE(match->expression, free_alloc);
    free_match_arm_list(&match->arms, free_alloc);
    NODE_VIRTUAL_FREE(match->catch_all, free_alloc);

    free_alloc(match);
}

NODISCARD Status match_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "match "));

    MatchExpression* match = (MatchExpression*)node;
    ASSERT_EXPRESSION(match->expression);
    TRY(NODE_VIRTUAL_RECONSTRUCT(match->expression, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, " { "));

    assert(match->arms.data && match->arms.length > 0);
    ArrayListIterator it = array_list_iterator_init(&match->arms);
    MatchArm          arm;
    while (array_list_iterator_has_next(&it, &arm)) {
        ASSERT_EXPRESSION(arm.pattern);
        TRY(NODE_VIRTUAL_RECONSTRUCT(arm.pattern, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, " => "));
        ASSERT_STATEMENT(arm.dispatch);
        TRY(NODE_VIRTUAL_RECONSTRUCT(arm.dispatch, symbol_map, sb));

        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append_str_z(sb, "}"));

    if (match->catch_all) {
        ASSERT_STATEMENT(match->catch_all);
        TRY(string_builder_append_str_z(sb, " else "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(match->catch_all, symbol_map, sb));
    }
    return SUCCESS;
}

NODISCARD Status match_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    MatchExpression* match = (MatchExpression*)node;
    ASSERT_EXPRESSION(match->expression);
    assert(match->arms.data && match->arms.length > 0);

    MAYBE_UNUSED(match);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
