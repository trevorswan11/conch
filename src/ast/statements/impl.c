#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/block.h"
#include "ast/statements/impl.h"

NODISCARD Status impl_statement_create(Token                 start_token,
                                       IdentifierExpression* parent,
                                       BlockStatement*       implementation,
                                       ImplStatement**       impl_stmt,
                                       memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(implementation->statements.length > 0);

    ImplStatement* impl = memory_alloc(sizeof(ImplStatement));
    if (!impl) {
        return ALLOCATION_FAILED;
    }

    *impl = (ImplStatement){
        .base           = STATEMENT_INIT(IMPL_VTABLE, start_token),
        .parent         = parent,
        .implementation = implementation,
    };

    *impl_stmt = impl;
    return SUCCESS;
}

void impl_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ImplStatement* impl = (ImplStatement*)node;
    NODE_VIRTUAL_FREE(impl->parent, free_alloc);
    NODE_VIRTUAL_FREE(impl->implementation, free_alloc);

    free_alloc(impl);
}

NODISCARD Status impl_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "impl "));

    ImplStatement* impl = (ImplStatement*)node;
    TRY(identifier_expression_reconstruct((Node*)impl->parent, symbol_map, sb));
    TRY(string_builder_append(sb, ' '));
    TRY(block_statement_reconstruct((Node*)impl->implementation, symbol_map, sb));

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}
