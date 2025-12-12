#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/infix.h"

#include "semantic/context.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

NODISCARD Status infix_expression_create(Token             start_token,
                                         Expression*       lhs,
                                         TokenType         op,
                                         Expression*       rhs,
                                         InfixExpression** infix_expr,
                                         memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    assert(lhs && rhs);

    InfixExpression* infix = memory_alloc(sizeof(InfixExpression));
    if (!infix) {
        return ALLOCATION_FAILED;
    }

    *infix = (InfixExpression){
        .base = EXPRESSION_INIT(INFIX_VTABLE, start_token),
        .lhs  = lhs,
        .op   = op,
        .rhs  = rhs,
    };

    *infix_expr = infix;
    return SUCCESS;
}

void infix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    InfixExpression* infix = (InfixExpression*)node;
    NODE_VIRTUAL_FREE(infix->lhs, free_alloc);
    NODE_VIRTUAL_FREE(infix->rhs, free_alloc);

    free_alloc(infix);
}

NODISCARD Status infix_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    InfixExpression* infix = (InfixExpression*)node;
    assert(infix->lhs && infix->rhs);

    Node* lhs = (Node*)infix->lhs;
    Slice op  = poll_tt_symbol(symbol_map, infix->op);
    Node* rhs = (Node*)infix->rhs;

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    TRY(lhs->vtable->reconstruct(lhs, symbol_map, sb));

    TRY(string_builder_append(sb, ' '));
    TRY(string_builder_append_slice(sb, op));
    TRY(string_builder_append(sb, ' '));

    TRY(rhs->vtable->reconstruct(rhs, symbol_map, sb));
    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status infix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
