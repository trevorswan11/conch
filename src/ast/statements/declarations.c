#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"

TRY_STATUS decl_statement_create(Token                 start_token,
                                 IdentifierExpression* ident,
                                 TypeExpression*       type,
                                 Expression*           value,
                                 DeclStatement**       decl_stmt,
                                 memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);

    if (!type) {
        return DECL_MISSING_TYPE;
    } else if (start_token.type != CONST && start_token.type != VAR) {
        return UNEXPECTED_TOKEN;
    }

    // There's some behavior to check for wit uninitialized decls
    if (!value) {
        if (start_token.type == CONST) {
            return CONST_DECL_MISSING_VALUE;
        } else if (start_token.type == VAR && type->type.tag == IMPLICIT) {
            return FORWARD_VAR_DECL_MISSING_TYPE;
        }
    }

    DeclStatement* declaration = memory_alloc(sizeof(DeclStatement));
    if (!declaration) {
        return ALLOCATION_FAILED;
    }

    *declaration = (DeclStatement){
        .base     = STATEMENT_INIT(DECL_VTABLE, start_token),
        .ident    = ident,
        .is_const = start_token.type == CONST,
        .type     = type,
        .value    = value,
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

TRY_STATUS decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, node->start_token.slice));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));

    DeclStatement* d          = (DeclStatement*)node;
    Node*          ident_node = (Node*)d->ident;
    PROPAGATE_IF_ERROR(ident_node->vtable->reconstruct(ident_node, symbol_map, sb));

    Node* type_node = (Node*)d->type;
    PROPAGATE_IF_ERROR(type_node->vtable->reconstruct(type_node, symbol_map, sb));

    if (d->value) {
        if (d->type->type.tag == EXPLICIT) {
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " = "));
        } else {
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "= "));
        }
        Node* value_node = (Node*)d->value;
        PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}

TRY_STATUS type_decl_statement_create(Token                 start_token,
                                      IdentifierExpression* ident,
                                      Expression*           value,
                                      bool                  primitive_alias,
                                      TypeDeclStatement**   type_decl_stmt,
                                      memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);

    if (!ident || !value) {
        return NULL_PARAMETER;
    }

    TypeDeclStatement* declaration = memory_alloc(sizeof(TypeDeclStatement));
    if (!declaration) {
        return ALLOCATION_FAILED;
    }

    *declaration = (TypeDeclStatement){
        .base            = STATEMENT_INIT(TYPE_DECL_VTABLE, start_token),
        .ident           = ident,
        .value           = value,
        .primitive_alias = primitive_alias,
    };

    *type_decl_stmt = declaration;
    return SUCCESS;
}

void type_decl_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    TypeDeclStatement* d = (TypeDeclStatement*)node;

    if (d->ident) {
        identifier_expression_destroy((Node*)d->ident, free_alloc);
        d->ident = NULL;
    }

    NODE_VIRTUAL_FREE(d->value, free_alloc);
    d->value = NULL;
}

TRY_STATUS
type_decl_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "type "));

    TypeDeclStatement* d          = (TypeDeclStatement*)node;
    Node*              ident_node = (Node*)d->ident;
    PROPAGATE_IF_ERROR(ident_node->vtable->reconstruct(ident_node, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " = "));
    Node* value_node = (Node*)d->value;
    PROPAGATE_IF_ERROR(value_node->vtable->reconstruct(value_node, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
