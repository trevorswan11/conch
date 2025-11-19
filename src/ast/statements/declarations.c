#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

TRY_STATUS decl_statement_create(Token                 token,
                                 IdentifierExpression* ident,
                                 Expression*           value,
                                 DeclStatement**       decl_stmt,
                                 memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(token.slice.ptr);
    DeclStatement* declaration = memory_alloc(sizeof(DeclStatement));
    if (!declaration) {
        return ALLOCATION_FAILED;
    }

    *declaration = (DeclStatement){
        .base  = STATEMENT_INIT(DECL_VTABLE),
        .token = token,
        .ident = ident,
        .value = value,
    };

    *decl_stmt = declaration;
    return SUCCESS;
}

void decl_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    DeclStatement* d = (DeclStatement*)node;

    if (d->ident) {
        Node* n_ident = (Node*)d->ident;
        n_ident->vtable->destroy(n_ident, free_alloc);
        d->ident = NULL;
    }

    if (d->value) {
        Node* n_value = (Node*)d->value;
        n_value->vtable->destroy(n_value, free_alloc);
        d->value = NULL;
    }

    free_alloc(d);
}

Slice decl_statement_token_literal(Node* node) {
    ASSERT_NODE(node);
    DeclStatement* d = (DeclStatement*)node;
    assert(d->token.type == CONST || d->token.type == VAR);

    const char* literal = d->token.type == CONST ? "const" : "var";
    return (Slice){
        .ptr    = literal,
        .length = strlen(literal),
    };
}

TRY_STATUS decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    DeclStatement* d = (DeclStatement*)node;
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, d->token.slice.ptr, d->token.slice.length));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));

    Node* ident_node = (Node*)d->ident;
    PROPAGATE_IF_ERROR(ident_node->vtable->reconstruct(ident_node, symbol_map, sb));
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, " = ", 3));

    if (d->value) {
        Node* value_node = (Node*)d->value;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
