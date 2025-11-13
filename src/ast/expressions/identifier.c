#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ast/expressions/identifier.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS identifier_expression_create(Slice                  name,
                                        IdentifierExpression** ident_expr,
                                        memory_alloc_fn        memory_alloc,
                                        free_alloc_fn          free_alloc) {
    assert(memory_alloc && free_alloc);
    IdentifierExpression* ident = memory_alloc(sizeof(IdentifierExpression));
    if (!ident) {
        return ALLOCATION_FAILED;
    }

    char* mut_name = strdup_s_allocator(name.ptr, name.length, memory_alloc);
    if (!mut_name) {
        free_alloc(ident);
        return ALLOCATION_FAILED;
    }

    *ident = (IdentifierExpression){
        .base =
            (Expression){
                .base =
                    (Node){
                        .vtable = &IDENTIFIER_VTABLE.base,
                    },
                .vtable = &IDENTIFIER_VTABLE,
            },
        .name =
            (MutSlice){
                .ptr    = mut_name,
                .length = strlen(mut_name),
            },
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

Slice identifier_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    MAYBE_UNUSED(node);

    const char* str = token_type_name(IDENT);
    return (Slice){
        .ptr    = str,
        .length = strlen(str),
    };
}

TRY_STATUS identifier_expression_reconstruct(Node* node, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    IdentifierExpression* ident = (IdentifierExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_mut_slice(sb, ident->name));
    return SUCCESS;
}

TRY_STATUS identifier_expression_node(Expression* expr) {
    ASSERT_EXPRESSION(expr);
    MAYBE_UNUSED(expr);
    return SUCCESS;
}
