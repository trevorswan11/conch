#include <stdlib.h>
#include <string.h>

#include "ast/expressions/identifier.h"

#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS identifier_expression_create(Slice name, IdentifierExpression** ident_expr) {
    IdentifierExpression* ident = malloc(sizeof(IdentifierExpression));
    if (!ident) {
        return ALLOCATION_FAILED;
    }

    char* mut_name = strdup_s(name.ptr, name.length);
    if (!mut_name) {
        free(ident);
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

void identifier_expression_destroy(Node* node) {
    ASSERT_NODE(node);
    IdentifierExpression* ident = (IdentifierExpression*)node;
    free(ident->name.ptr);
    free(ident);
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
