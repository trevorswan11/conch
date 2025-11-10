#include <stdio.h>
#include <stdlib.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"

DeclStatement*
decl_statement_create(IdentifierExpression* ident, Expression* value, bool constant) {
    DeclStatement* declaration = malloc(sizeof(DeclStatement));
    if (!declaration) {
        return NULL;
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

    return declaration;
}
