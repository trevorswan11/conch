#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"

VarStatement* var_statement_create(IdentifierExpression* ident, Expression* value) {
    VarStatement* var = malloc(sizeof(VarStatement));
    if (!var) {
        return NULL;
    }

    *var = (VarStatement){
        .base =
            (Statement){
                .base.vtable = &VAR_VTABLE.base,
                .vtable      = &VAR_VTABLE,
            },
        .ident = ident,
        .value = value,
    };

    return var;
}
