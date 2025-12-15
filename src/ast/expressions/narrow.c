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
    ASSERT_NODE(node);
    assert(free_alloc);

    NarrowExpression* narrow = (NarrowExpression*)node;
    NODE_VIRTUAL_FREE(narrow->outer, free_alloc);
    NODE_VIRTUAL_FREE(narrow->inner, free_alloc);

    free_alloc(narrow);
}

NODISCARD Status narrow_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    NarrowExpression* narrow = (NarrowExpression*)node;
    assert(narrow->outer && narrow->inner);

    Node* outer = (Node*)narrow->outer;
    Node* inner = (Node*)narrow->inner;

    TRY(outer->vtable->reconstruct(outer, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, "::"));
    TRY(inner->vtable->reconstruct(inner, symbol_map, sb));

    return SUCCESS;
}

NODISCARD Status narrow_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
