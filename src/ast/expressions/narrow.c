#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/narrow.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

NODISCARD Status narrow_expression_create(Token                 start_token,
                                          Expression*           outer,
                                          IdentifierExpression* inner,
                                          NarrowExpression**    narrow_expr,
                                          memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(outer && inner);

    NarrowExpression* narrow = memory_alloc(sizeof(NarrowExpression));
    if (!narrow) {
        return ALLOCATION_FAILED;
    }

    *narrow = (NarrowExpression){
        .base  = EXPRESSION_INIT(NARROW_VTABLE, start_token),
        .outer = outer,
        .inner = inner,
    };

    *narrow_expr = narrow;
    return SUCCESS;
}

void narrow_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    NarrowExpression* narrow = (NarrowExpression*)node;
    NODE_VIRTUAL_FREE(narrow->outer, free_alloc);
    NODE_VIRTUAL_FREE(narrow->inner, free_alloc);

    free_alloc(narrow);
}

NODISCARD Status narrow_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    NarrowExpression* narrow = (NarrowExpression*)node;

    ASSERT_EXPRESSION(narrow->outer);
    TRY(NODE_VIRTUAL_RECONSTRUCT(narrow->outer, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, "::"));
    ASSERT_EXPRESSION(narrow->inner);
    TRY(NODE_VIRTUAL_RECONSTRUCT(narrow->inner, symbol_map, sb));

    return SUCCESS;
}

NODISCARD Status narrow_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    NarrowExpression* narrow = (NarrowExpression*)node;
    ASSERT_EXPRESSION(narrow->outer);
    ASSERT_EXPRESSION(narrow->inner);

    MAYBE_UNUSED(narrow);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
