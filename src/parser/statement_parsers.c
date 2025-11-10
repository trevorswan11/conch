#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

DeclStatement* decl_statement_parse(Parser* p, bool constant) {
    DeclStatement* stmt = decl_statement_create(NULL, NULL, constant);
    if (!stmt) {
        return NULL;
    }

    if (!parser_expect_peek(p, IDENT)) {
        decl_statement_destroy((Node*)stmt);
        return NULL;
    }
    stmt->ident = identifier_expression_create(p->current_token.slice);

    if (!parser_expect_peek(p, WALRUS)) {
        decl_statement_destroy((Node*)stmt);
        return NULL;
    }

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        if (!parser_next_token(p)) {
            decl_statement_destroy((Node*)stmt);
            return NULL;
        }
    }

    return stmt;
}

ReturnStatement* return_statement_parse(Parser* p) {
    ReturnStatement* stmt = return_statement_create(NULL);
    if (!stmt) {
        return NULL;
    }

    if (!parser_next_token(p)) {
        return_statement_destroy((Node*)stmt);
        return NULL;
    }

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        if (!parser_next_token(p)) {
            return_statement_destroy((Node*)stmt);
            return NULL;
        }
    }

    return stmt;
}
