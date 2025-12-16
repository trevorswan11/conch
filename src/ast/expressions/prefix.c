#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/prefix.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

NODISCARD Status prefix_expression_create(Token              start_token,
                                          Expression*        rhs,
                                          PrefixExpression** prefix_expr,
                                          memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(rhs);

    PrefixExpression* prefix = memory_alloc(sizeof(PrefixExpression));
    if (!prefix) {
        return ALLOCATION_FAILED;
    }

    *prefix = (PrefixExpression){
        .base = EXPRESSION_INIT(PREFIX_VTABLE, start_token),
        .rhs  = rhs,
    };

    *prefix_expr = prefix;
    return SUCCESS;
}

void prefix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    PrefixExpression* prefix = (PrefixExpression*)node;
    NODE_VIRTUAL_FREE(prefix->rhs, free_alloc);

    free_alloc(prefix);
}

NODISCARD Status prefix_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    PrefixExpression* prefix = (PrefixExpression*)node;
    Slice             op     = poll_tt_symbol(symbol_map, node->start_token.type);

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    TRY(string_builder_append_slice(sb, op));

    ASSERT_EXPRESSION(prefix->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(prefix->rhs, symbol_map, sb));

    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status prefix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    PrefixExpression* prefix = (PrefixExpression*)node;
    ASSERT_EXPRESSION(prefix->rhs);

    MAYBE_UNUSED(prefix);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
