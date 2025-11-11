#include <stdlib.h>
#include <string.h>

#include "ast/expressions/identifier.h"

#include "util/error.h"
#include "util/mem.h"

AnyError identifier_expression_create(Slice name, IdentifierExpression** ident_expr) {
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
                .base.vtable = &IDENTIFIER_VTABLE.base,
                .vtable      = &IDENTIFIER_VTABLE,
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
