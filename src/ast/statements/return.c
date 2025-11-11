#include <stdio.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/statements/return.h"

#include "util/error.h"

AnyError return_statement_create(Expression* value, ReturnStatement** ret_stmt) {
    ReturnStatement* ret = malloc(sizeof(ReturnStatement));
    if (!ret) {
        return ALLOCATION_FAILED;
    }

    *ret = (ReturnStatement){
        .base =
            (Statement){
                .base.vtable = &RET_VTABLE.base,
                .vtable      = &RET_VTABLE,
            },
        .value = value,
    };

    *ret_stmt = ret;
    return SUCCESS;
}
