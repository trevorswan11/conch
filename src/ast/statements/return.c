#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/statements/return.h"

ReturnStatement* return_statement_create(Expression* value) {
    ReturnStatement* ret = malloc(sizeof(ReturnStatement));
    if (!ret) {
        return NULL;
    }

    *ret = (ReturnStatement){
        .base =
            (Statement){
                .base.vtable = &RET_VTABLE.base,
                .vtable      = &RET_VTABLE,
            },
        .value = value,
    };

    return ret;
}
