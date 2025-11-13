#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

#include "util/allocator.h"
#include "util/status.h"

TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression) {
    IdentifierExpression* ident;
    PROPAGATE_IF_ERROR(identifier_expression_create(
        p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc));
    *expression = (Expression*)ident;
    return SUCCESS;
}
