#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/block.h"
#include "ast/statements/impl.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status impl_statement_create(Token                 start_token,
                                           IdentifierExpression* parent,
                                           ArrayList             generics,
                                           BlockStatement*       implementation,
                                           ImplStatement**       impl_stmt,
                                           Allocator*            allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ASSERT_EXPRESSION(parent);
    ASSERT_STATEMENT(implementation);
    assert(implementation->statements.length > 0);

    ImplStatement* impl = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*impl));
    if (!impl) { return ALLOCATION_FAILED; }

    *impl = (ImplStatement){
        .base           = STATEMENT_INIT(IMPL_VTABLE, start_token),
        .parent         = parent,
        .generics       = generics,
        .implementation = implementation,
    };

    *impl_stmt = impl;
    return SUCCESS;
}

void impl_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    ImplStatement* impl = (ImplStatement*)node;
    NODE_VIRTUAL_FREE(impl->parent, allocator);
    free_expression_list(&impl->generics, allocator);
    NODE_VIRTUAL_FREE(impl->implementation, allocator);

    ALLOCATOR_PTR_FREE(allocator, impl);
}

[[nodiscard]] Status
impl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "impl "));

    ImplStatement* impl = (ImplStatement*)node;
    ASSERT_EXPRESSION(impl->parent);
    TRY(NODE_VIRTUAL_RECONSTRUCT(impl->parent, symbol_map, sb));
    TRY(generics_reconstruct(&impl->generics, symbol_map, sb));
    TRY(string_builder_append(sb, ' '));
    ASSERT_STATEMENT(impl->implementation);
    TRY(NODE_VIRTUAL_RECONSTRUCT(impl->implementation, symbol_map, sb));

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status impl_statement_analyze(Node*                             node,
                                            [[maybe_unused]] SemanticContext* parent,
                                            [[maybe_unused]] ArrayList*       errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    [[maybe_unused]] ImplStatement* impl = (ImplStatement*)node;
    ASSERT_EXPRESSION(impl->parent);
    ASSERT_STATEMENT(impl->implementation);
    assert(impl->implementation->statements.data && impl->implementation->statements.length > 0);

    return NOT_IMPLEMENTED;
}
