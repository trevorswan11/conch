#include <assert.h>

#include "ast/expressions/match.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_match_arm_list(ArrayList* arms, Allocator* allocator) {
    assert(arms && arms->item_size == sizeof(MatchArm));
    ASSERT_ALLOCATOR_PTR(allocator);

    ArrayListConstIterator it = array_list_const_iterator_init(arms);
    MatchArm               arm;
    while (array_list_const_iterator_has_next(&it, &arm)) {
        NODE_VIRTUAL_FREE(arm.pattern, allocator);
        NODE_VIRTUAL_FREE(arm.dispatch, allocator);
    }

    array_list_deinit(arms);
}

[[nodiscard]] Status match_expression_create(Token             start_token,
                                             Expression*       expression,
                                             ArrayList         arms,
                                             Statement*        catch_all,
                                             MatchExpression** match_expr,
                                             Allocator*        allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ASSERT_EXPRESSION(expression);
    assert(arms.item_size == sizeof(MatchArm));
    assert(arms.length > 0);

    MatchExpression* match = ALLOCATOR_PTR_MALLOC(allocator, sizeof(MatchExpression));
    if (!match) { return ALLOCATION_FAILED; }

    *match = (MatchExpression){
        .base       = EXPRESSION_INIT(MATCH_VTABLE, start_token),
        .expression = expression,
        .arms       = arms,
        .catch_all  = catch_all,
    };

    *match_expr = match;
    return SUCCESS;
}

void match_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    MatchExpression* match = (MatchExpression*)node;
    NODE_VIRTUAL_FREE(match->expression, allocator);
    free_match_arm_list(&match->arms, allocator);
    NODE_VIRTUAL_FREE(match->catch_all, allocator);

    ALLOCATOR_PTR_FREE(allocator, match);
}

[[nodiscard]] Status
match_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "match "));

    MatchExpression* match = (MatchExpression*)node;
    ASSERT_EXPRESSION(match->expression);
    TRY(NODE_VIRTUAL_RECONSTRUCT(match->expression, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, " { "));

    assert(match->arms.data && match->arms.length > 0);
    ArrayListConstIterator it = array_list_const_iterator_init(&match->arms);
    MatchArm               arm;
    while (array_list_const_iterator_has_next(&it, &arm)) {
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

[[nodiscard]] Status match_expression_analyze(Node*                             node,
                                              [[maybe_unused]] SemanticContext* parent,
                                              [[maybe_unused]] ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    [[maybe_unused]] MatchExpression* match = (MatchExpression*)node;
    ASSERT_EXPRESSION(match->expression);
    assert(match->arms.data && match->arms.length > 0);

    return NOT_IMPLEMENTED;
}
