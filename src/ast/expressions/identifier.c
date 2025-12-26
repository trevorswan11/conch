#include <assert.h>

#include "ast/expressions/identifier.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status identifier_expression_create(Token                  start_token,
                                                  MutSlice               name,
                                                  IdentifierExpression** ident_expr,
                                                  memory_alloc_fn        memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr && start_token.slice.length > 0);

    IdentifierExpression* ident = memory_alloc(sizeof(IdentifierExpression));
    if (!ident) { return ALLOCATION_FAILED; }

    *ident = (IdentifierExpression){
        .base = EXPRESSION_INIT(IDENTIFIER_VTABLE, start_token),
        .name = name,
    };

    *ident_expr = ident;
    return SUCCESS;
}

void identifier_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    free_alloc(ident->name.ptr);

    free_alloc(ident);
}

[[nodiscard]] Status identifier_expression_reconstruct(Node*                           node,
                                                       [[maybe_unused]] const HashMap* symbol_map,
                                                       StringBuilder*                  sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    assert(ident->name.ptr && ident->name.length > 0);
    TRY(string_builder_append_mut_slice(sb, ident->name));
    return SUCCESS;
}

[[nodiscard]] Status
identifier_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    assert(ident->name.ptr && ident->name.length > 0);

    // Identifiers are only analyzed if they have been declared
    SemanticType* semantic_type;
    if (!semantic_context_find(parent, true, ident->name, &semantic_type)) {
        const Token start_token = node->start_token;
        PUT_STATUS_PROPAGATE(errors, UNDECLARED_IDENTIFIER, start_token, {});
    }

    parent->analyzed_type = rc_retain(semantic_type);
    return SUCCESS;
}
