#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"
#include "util/containers/string_builder.h"

NODISCARD Status decl_statement_create(Token                 start_token,
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

    DeclStatement* decl = (DeclStatement*)node;
    NODE_VIRTUAL_FREE(decl->ident, free_alloc);
    NODE_VIRTUAL_FREE(decl->type, free_alloc);
    NODE_VIRTUAL_FREE(decl->value, free_alloc);

    free_alloc(decl);
}

NODISCARD Status decl_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    TRY(string_builder_append(sb, ' '));

    DeclStatement* d          = (DeclStatement*)node;
    Node*          ident_node = (Node*)d->ident;
    TRY(ident_node->vtable->reconstruct(ident_node, symbol_map, sb));
    if (d->type->type.tag == EXPLICIT) {
        TRY(string_builder_append_str_z(sb, ": "));
    }

    Node* type_node = (Node*)d->type;
    TRY(type_node->vtable->reconstruct(type_node, symbol_map, sb));

    if (d->value) {
        if (d->type->type.tag == EXPLICIT) {
            TRY(string_builder_append_str_z(sb, " = "));
        } else {
            TRY(string_builder_append_str_z(sb, "= "));
        }
        Node* value_node = (Node*)d->value;
        TRY(value_node->vtable->reconstruct(value_node, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

NODISCARD Status type_decl_statement_create(Token                 start_token,
                                            IdentifierExpression* ident,
                                            Expression*           value,
                                            bool                  primitive_alias,
                                            TypeDeclStatement**   type_decl_stmt,
                                            memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(ident && value);

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

    TypeDeclStatement* type_decl = (TypeDeclStatement*)node;
    NODE_VIRTUAL_FREE(type_decl->ident, free_alloc);
    NODE_VIRTUAL_FREE(type_decl->value, free_alloc);

    free_alloc(type_decl);
}

NODISCARD Status type_decl_statement_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "type "));

    TypeDeclStatement* d          = (TypeDeclStatement*)node;
    Node*              ident_node = (Node*)d->ident;
    TRY(ident_node->vtable->reconstruct(ident_node, symbol_map, sb));

    TRY(string_builder_append_str_z(sb, " = "));
    Node* value_node = (Node*)d->value;
    TRY(value_node->vtable->reconstruct(value_node, symbol_map, sb));

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}
