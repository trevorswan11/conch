#include <stdio.h>
#include <stdlib.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"

#include "util/error.h"

AnyError decl_statement_create(IdentifierExpression* ident,
                               Expression*           value,
                               bool                  constant,
                               DeclStatement**       decl_stmt) {
    DeclStatement* declaration = malloc(sizeof(DeclStatement));
    if (!declaration) {
        return ALLOCATION_FAILED;
    }

    *declaration = (DeclStatement){
        .base =
            (Statement){
                .base.vtable = &DECL_VTABLE.base,
                .vtable      = &DECL_VTABLE,
            },
        .ident    = ident,
        .value    = value,
        .constant = constant,
    };

    *decl_stmt = declaration;
    return SUCCESS;
}
