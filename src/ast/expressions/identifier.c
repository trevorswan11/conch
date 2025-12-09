#include <assert.h>

#include "ast/expressions/identifier.h"

NODISCARD Status identifier_expression_create(Token                  start_token,
                                              MutSlice               name,
                                              IdentifierExpression** ident_expr,
                                              memory_alloc_fn        memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    IdentifierExpression* ident = memory_alloc(sizeof(IdentifierExpression));
    if (!ident) {
        return ALLOCATION_FAILED;
    }

    *ident = (IdentifierExpression){
        .base = EXPRESSION_INIT(IDENTIFIER_VTABLE, start_token),
        .name = name,
    };

    *ident_expr = ident;
    return SUCCESS;
}

void identifier_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    free_alloc(ident->name.ptr);

    free_alloc(ident);
}

NODISCARD Status identifier_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    MAYBE_UNUSED(symbol_map);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    TRY(string_builder_append_mut_slice(sb, ident->name));
    return SUCCESS;
}
