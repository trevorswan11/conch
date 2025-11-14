#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "util/allocator.h"
#include "util/containers/hash_set.h"
#include "util/status.h"

const char* precedence_name(Precedence precedence) {
    return PRECEDENCE_NAMES[precedence];
}

TRY_STATUS expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression) {
    assert(p);

    PrefixFn prefix;
    if (!poll_prefix(p, p->current_token.type, &prefix)) {
        return ELEMENT_MISSING;
    }

    MAYBE_UNUSED(precedence);
    PROPAGATE_IF_ERROR(prefix.prefix_parse(p, lhs_expression));
    return SUCCESS;
}

TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression) {
    IdentifierExpression* ident;
    PROPAGATE_IF_ERROR(identifier_expression_create(
        p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc));
    *expression = (Expression*)ident;
    return SUCCESS;
}
