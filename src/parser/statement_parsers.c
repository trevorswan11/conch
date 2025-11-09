#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

VarStatement* var_statement_parse(Parser* p) {
    VarStatement* stmt = var_statement_create(NULL, NULL);
    if (!stmt) {
        return NULL;
    }

    if (!parser_expect_peek(p, IDENT)) {
        var_statement_destroy((Node*)stmt);
        return NULL;
    }
    stmt->ident = identifier_expression_create(p->current_token.slice);

    if (!parser_expect_peek(p, WALRUS)) {
        var_statement_destroy((Node*)stmt);
        return NULL;
    }

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        if (!parser_next_token(p)) {
            var_statement_destroy((Node*)stmt);
            return NULL;
        }
    }

    return stmt;
}
