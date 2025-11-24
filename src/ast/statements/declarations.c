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
                                 TypeExpression*       type,
                                 Expression*           value,
                                 DeclStatement**       decl_stmt,
                                 memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(token.slice.ptr);
    if (!type) {
        return DECL_MISSING_TYPE;
    }
    if (token.type != CONST && token.type != VAR) {
        return UNEXPECTED_TOKEN;
    }

    // Theres some behavior to check for wit uninitialized decls
    if (!value) {
        if (token.type == CONST) {
            return CONST_DECL_MISSING_VALUE;
        } else if (token.type == VAR && type->type.tag == IMPLICIT) {
            return FORWARD_VAR_DECL_MISSING_TYPE;
        }
    }

    DeclStatement* declaration = memory_alloc(sizeof(DeclStatement));
    if (!declaration) {
        return ALLOCATION_FAILED;
    }

    *declaration = (DeclStatement){
        .base  = STATEMENT_INIT(DECL_VTABLE),
        .token = token,
        .ident = ident,
        .type  = type,
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
        identifier_expression_destroy((Node*)d->ident, free_alloc);
        d->ident = NULL;
    }

    if (d->type) {
        type_expression_destroy((Node*)d->type, free_alloc);
        d->type = NULL;
    }

    NODE_VIRTUAL_FREE(d->value, free_alloc);
    d->value = NULL;

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

    Node* type_node = (Node*)d->type;
    PROPAGATE_IF_ERROR(type_node->vtable->reconstruct(type_node, symbol_map, sb));

    if (d->value) {
        if (d->type->type.tag == EXPLICIT) {
            PROPAGATE_IF_ERROR(string_builder_append_many(sb, " = ", 3));
        } else {
            PROPAGATE_IF_ERROR(string_builder_append_many(sb, "= ", 2));
        }
        Node* value_node = (Node*)d->value;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
