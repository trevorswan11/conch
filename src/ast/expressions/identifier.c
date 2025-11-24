#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/token.h"

#include "ast/expressions/identifier.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS identifier_expression_create(Token                  token,
                                        IdentifierExpression** ident_expr,
                                        memory_alloc_fn        memory_alloc,
                                        free_alloc_fn          free_alloc) {
    assert(memory_alloc && free_alloc);
    assert(token.slice.ptr);
    IdentifierExpression* ident = memory_alloc(sizeof(IdentifierExpression));
    if (!ident) {
        return ALLOCATION_FAILED;
    }

    char* mut_name = strdup_s_allocator(token.slice.ptr, token.slice.length, memory_alloc);
    if (!mut_name) {
        free_alloc(ident);
        return ALLOCATION_FAILED;
    }

    *ident = (IdentifierExpression){
        .base       = EXPRESSION_INIT(IDENTIFIER_VTABLE),
        .name       = mut_slice_from_str_z(mut_name),
        .token_type = token.type,
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
    IdentifierExpression* ident = (IdentifierExpression*)node;
    return slice_from_str_z(token_type_name(ident->token_type));
}

TRY_STATUS
identifier_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    IdentifierExpression* ident = (IdentifierExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_mut_slice(sb, ident->name));
    return SUCCESS;
}
