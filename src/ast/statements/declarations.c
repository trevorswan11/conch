#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

#define DECL_NAME(tok, field, name)                                                     \
    const Allocator allocator = semantic_context_allocator(parent);                     \
    MutSlice        name;                                                               \
    TRY(mut_slice_dupe(&(name), &(field), allocator.memory_alloc));                     \
                                                                                        \
    if (semantic_context_has(parent, true, (name))) {                                   \
        PUT_STATUS_PROPAGATE(                                                           \
            errors, REDEFINITION_OF_IDENTIFIER, tok, allocator.free_alloc((name).ptr)); \
    }

[[nodiscard]] Status decl_statement_create(Token                 start_token,
                                       IdentifierExpression* ident,
                                       TypeExpression*       type,
                                       Expression*           value,
                                       DeclStatement**       decl_stmt,
                                       memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    ASSERT_EXPRESSION(ident);
    ASSERT_EXPRESSION(type);
    assert(start_token.type == CONST || start_token.type == VAR);

    // We really can't allow the type to be a typeof expression
    if (type->tag == EXPLICIT && type->variant.explicit_type.tag == EXPLICIT_TYPEOF) {
        return ILLEGAL_DECL_CONSTRUCT;
    }

    // There's some behavior to check for with uninitialized decls
    if (!value) {
        if (start_token.type == CONST) {
            assert(type->tag == EXPLICIT);
            return CONST_DECL_MISSING_VALUE;
        }

        if (start_token.type == VAR && type->tag == IMPLICIT) {
            return FORWARD_VAR_DECL_MISSING_TYPE;
        }
    } else {
        ASSERT_EXPRESSION(value);
    }

    DeclStatement* declaration = memory_alloc(sizeof(DeclStatement));
    if (!declaration) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    DeclStatement* decl = (DeclStatement*)node;
    NODE_VIRTUAL_FREE(decl->ident, free_alloc);
    NODE_VIRTUAL_FREE(decl->type, free_alloc);
    NODE_VIRTUAL_FREE(decl->value, free_alloc);

    free_alloc(decl);
}

[[nodiscard]] Status decl_statement_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    TRY(string_builder_append(sb, ' '));

    DeclStatement* decl = (DeclStatement*)node;
    ASSERT_EXPRESSION(decl->ident);
    TRY(NODE_VIRTUAL_RECONSTRUCT(decl->ident, symbol_map, sb));
    if (decl->type->tag == EXPLICIT) { TRY(string_builder_append_str_z(sb, ": ")); }

    TRY(NODE_VIRTUAL_RECONSTRUCT(decl->type, symbol_map, sb));
    if (decl->value) {
        ASSERT_EXPRESSION(decl->value);
        if (decl->type->tag == EXPLICIT) {
            TRY(string_builder_append_str_z(sb, " = "));
        } else {
            TRY(string_builder_append_str_z(sb, "= "));
        }
        TRY(NODE_VIRTUAL_RECONSTRUCT(decl->value, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status decl_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    DeclStatement* decl = (DeclStatement*)node;
    ASSERT_EXPRESSION(decl->ident);
    ASSERT_EXPRESSION(decl->type);

    const Token ident_token = NODE_TOKEN(decl->ident);
    DECL_NAME(ident_token, decl->ident->name, decl_name);

    TRY_DO(NODE_VIRTUAL_ANALYZE(decl->type, parent, errors), allocator.free_alloc(decl_name.ptr));
    SemanticType* analyzed_decl_type = semantic_context_move_analyzed(parent);

    SemanticType* decl_type;
    TRY_DO(semantic_type_copy(&decl_type, analyzed_decl_type, allocator), {
        allocator.free_alloc(decl_name.ptr);
        RC_RELEASE(analyzed_decl_type, allocator.free_alloc);
    });
    RC_RELEASE(analyzed_decl_type, allocator.free_alloc);

    if (decl->value) {
        TRY_DO(NODE_VIRTUAL_ANALYZE(decl->value, parent, errors), {
            allocator.free_alloc(decl_name.ptr);
            RC_RELEASE(decl_type, allocator.free_alloc);
        });

        SemanticType* value_type = semantic_context_move_analyzed(parent);
        if (!value_type->valued) {
            const Token t = NODE_TOKEN(decl->value);
            PUT_STATUS_PROPAGATE(errors, NON_VALUED_DECL_VALUE, t, {
                allocator.free_alloc(decl_name.ptr);
                RC_RELEASE(decl_type, allocator.free_alloc);
                RC_RELEASE(value_type, allocator.free_alloc);
            });
        }

        if (decl_type->tag == STYPE_IMPLICIT_DECLARATION) {
            // We can now release the decl and just have it set to the value's type
            RC_RELEASE(decl_type, allocator.free_alloc);
            decl_type = value_type;
        } else {
            decl_type->valued = true;

            // In this case, we can drop the value type since all we need is the decl
            const bool assignable = type_assignable(decl_type, value_type);
            RC_RELEASE(value_type, allocator.free_alloc);

            if (!assignable) {
                const Token t = NODE_TOKEN(decl);
                PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, t, {
                    allocator.free_alloc(decl_name.ptr);
                    RC_RELEASE(decl_type, allocator.free_alloc);
                });
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
        allocator.free_alloc(decl_name.ptr);
        RC_RELEASE(decl_type, allocator.free_alloc);
    });

    // The table retains the type, so we can release this control flow's ownership
    RC_RELEASE(decl_type, allocator.free_alloc);
    return SUCCESS;
}

[[nodiscard]] Status type_decl_statement_create(Token                 start_token,
                                            IdentifierExpression* ident,
                                            Expression*           value,
                                            bool                  primitive_alias,
                                            TypeDeclStatement**   type_decl_stmt,
                                            memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(ident && value);

    TypeDeclStatement* declaration = memory_alloc(sizeof(TypeDeclStatement));
    if (!declaration) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    TypeDeclStatement* type_decl = (TypeDeclStatement*)node;
    NODE_VIRTUAL_FREE(type_decl->ident, free_alloc);
    NODE_VIRTUAL_FREE(type_decl->value, free_alloc);

    free_alloc(type_decl);
}

[[nodiscard]] Status type_decl_statement_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "type "));

    TypeDeclStatement* type_decl = (TypeDeclStatement*)node;
    ASSERT_EXPRESSION(type_decl->ident);
    TRY(NODE_VIRTUAL_RECONSTRUCT(type_decl->ident, symbol_map, sb));

    TRY(string_builder_append_str_z(sb, " = "));
    ASSERT_EXPRESSION(type_decl->value);
    TRY(NODE_VIRTUAL_RECONSTRUCT(type_decl->value, symbol_map, sb));

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status type_decl_statement_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    TypeDeclStatement* type_decl = (TypeDeclStatement*)node;
    ASSERT_EXPRESSION(type_decl->ident);
    ASSERT_EXPRESSION(type_decl->value);

    const Token ident_token = NODE_TOKEN(type_decl->ident);
    DECL_NAME(ident_token, type_decl->ident->name, type_decl_name);

    // Primitive aliases can be eagerly evaluated and added to the context
    if (type_decl->primitive_alias) {
        IdentifierExpression* primitive = (IdentifierExpression*)type_decl->value;
        SemanticTypeTag       primitive_type_tag;
        if (!semantic_name_to_primitive_type_tag(primitive->name, &primitive_type_tag)) {
            allocator.free_alloc(type_decl_name.ptr);
            return VIOLATED_INVARIANT;
        }

        // This helper has an error path from an allocation, and requires manual de-value-ing
        MAKE_PRIMITIVE(primitive_type_tag,
                       false,
                       primitive_type,
                       parent->symbol_table->symbols.allocator.memory_alloc,
                       allocator.free_alloc(type_decl_name.ptr));
        primitive_type->valued = false;

        // Now we just move the type into the context and release our ownership
        TRY_DO(symbol_table_add(parent->symbol_table, type_decl_name, primitive_type), {
            allocator.free_alloc(type_decl_name.ptr);
            RC_RELEASE(primitive_type, allocator.free_alloc);
        });

        RC_RELEASE(primitive_type, allocator.free_alloc);
        return SUCCESS;
    }

    // Otherwise the value has to be a type
    parent->namespace_type_name = slice_from_mut(&type_decl_name);
    TRY_DO(NODE_VIRTUAL_ANALYZE(type_decl->value, parent, errors),
           allocator.free_alloc(type_decl_name.ptr));
    parent->namespace_type_name = zeroed_slice();

    SemanticType* type_decl_type_probe = semantic_context_move_analyzed(parent);
    if (type_decl_type_probe->valued || !type_decl_type_probe->is_const) {
        const Token value_token = NODE_TOKEN(type_decl->ident);
        PUT_STATUS_PROPAGATE(errors, MALFORMED_TYPE_DECL, value_token, {
            allocator.free_alloc(type_decl_name.ptr);
            RC_RELEASE(type_decl_type_probe, allocator.free_alloc);
        });
    }

    TRY_DO(symbol_table_add(parent->symbol_table, type_decl_name, type_decl_type_probe), {
        allocator.free_alloc(type_decl_name.ptr);
        RC_RELEASE(type_decl_type_probe, allocator.free_alloc);
    });

    RC_RELEASE(type_decl_type_probe, allocator.free_alloc);
    return SUCCESS;
}
