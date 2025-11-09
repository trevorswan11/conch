#include <stdlib.h>
#include <string.h>

#include "ast/expressions/identifier.h"

#include "util/mem.h"

IdentifierExpression* identifier_expression_create(Slice name) {
    IdentifierExpression* ident = malloc(sizeof(IdentifierExpression));
    if (!ident) {
        return NULL;
    }

    char* mut_name = strdup_s(name.ptr, name.length);
    if (!mut_name) {
        free(ident);
        return NULL;
    }

    *ident = (IdentifierExpression){
        .base =
            (Expression){
                .base.vtable = &IDENTIFIER_VTABLE.base,
                .vtable      = &IDENTIFIER_VTABLE,
            },
        .name =
            (MutSlice){
                .ptr    = mut_name,
                .length = strlen(mut_name),
            },
    };

    return ident;
}
