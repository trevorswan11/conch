#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

#include "util/error.h"

AnyError decl_statement_parse(Parser* p, bool constant, DeclStatement** stmt) {
    DeclStatement* decl_stmt;
    PROPAGATE_IF_ERROR(decl_statement_create(NULL, NULL, constant, &decl_stmt));

    PROPAGATE_IF_ERROR_DO_IS(
        parser_expect_peek(p, IDENT), decl_statement_destroy((Node*)decl_stmt), UNEXPECTED_TOKEN);
    PROPAGATE_IF_ERROR_DO(identifier_expression_create(p->current_token.slice, &decl_stmt->ident),
                          decl_statement_destroy((Node*)decl_stmt));

    PROPAGATE_IF_ERROR_DO_IS(
        parser_expect_peek(p, WALRUS), decl_statement_destroy((Node*)decl_stmt), UNEXPECTED_TOKEN);

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), decl_statement_destroy((Node*)decl_stmt););
    }

    *stmt = decl_stmt;
    return SUCCESS;
}

AnyError return_statement_parse(Parser* p, ReturnStatement** stmt) {
    ReturnStatement* ret_stmt;
    PROPAGATE_IF_ERROR(return_statement_create(NULL, &ret_stmt));

    PROPAGATE_IF_ERROR_DO(parser_next_token(p), return_statement_destroy((Node*)ret_stmt));

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), return_statement_destroy((Node*)ret_stmt));
    }

    *stmt = ret_stmt;
    return SUCCESS;
}
