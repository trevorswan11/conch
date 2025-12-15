#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status decl_statement_create(Token                 start_token,
                                       IdentifierExpression* ident,
                                       TypeExpression*       type,
                                       Expression*           value,
                                       DeclStatement**       decl_stmt,
                                       memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(type);
    assert(start_token.type == CONST || start_token.type == VAR);

    // There's some behavior to check for with uninitialized decls
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

NODISCARD Status decl_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    DeclStatement* decl     = (DeclStatement*)node;
    Slice          tmp_name = slice_from_mut(&decl->ident->name);

    Allocator allocator = parent->symbol_table->symbols.allocator;
    char*     duped     = strdup_s_allocator(tmp_name.ptr, tmp_name.length, allocator.memory_alloc);
    if (!duped) {
        return ALLOCATION_FAILED;
    }
    const MutSlice decl_name = mut_slice_from_str_s(duped, tmp_name.length);

    // Declarations are checked in all parent scopes since there is no shadowing
    if (semantic_context_has(parent, true, decl_name)) {
        const Token start_token = NODE_TOKEN(decl->ident);
        IGNORE_STATUS(put_status_error(
            errors, REDEFINITION_OF_IDENTIFIER, start_token.line, start_token.column));

        allocator.free_alloc(duped);
        return REDEFINITION_OF_IDENTIFIER;
    }

    TRY_DO(NODE_VIRTUAL_ANALYZE(decl->type, parent, errors), allocator.free_alloc(duped));
    SemanticType* decl_type = semantic_context_move_analyzed(parent);

    if (decl->value) {
        TRY_DO(NODE_VIRTUAL_ANALYZE(decl->value, parent, errors), {
            allocator.free_alloc(duped);
            rc_release(decl_type, allocator.free_alloc);
        });
        SemanticType* value_type = semantic_context_move_analyzed(parent);
        if (decl_type->tag == STYPE_IMPLICIT_DECLARATION) {
            // We can now release the decl and just have it set to the value's type
            rc_release(decl_type, allocator.free_alloc);
            decl_type = value_type;
        } else {
            // In this case, we can drop the value type since all we need is the decl
            const bool same_type = type_check(decl_type, value_type);
            rc_release(value_type, allocator.free_alloc);

            if (!same_type) {
                const Token t = NODE_TOKEN(decl);
                IGNORE_STATUS(put_status_error(errors, TYPE_MISMATCH, t.line, t.column));

                allocator.free_alloc(duped);
                rc_release(decl_type, allocator.free_alloc);

                return TYPE_MISMATCH;
            }
        }
    } else {
        assert(!decl->is_const);
        assert(decl_type->tag != STYPE_IMPLICIT_DECLARATION);
    }

    // Enforce const qualifier, but don't return anything in the context
    decl_type->is_const = decl->is_const;
    assert(!parent->analyzed_type);

    TRY_DO(symbol_table_add(parent->symbol_table, decl_name, decl_type), {
        allocator.free_alloc(duped);
        rc_release(decl_type, allocator.free_alloc);
    });

    // The table retains the type, so we can release this control flow's ownership
    rc_release(decl_type, allocator.free_alloc);
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

NODISCARD Status type_decl_statement_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
