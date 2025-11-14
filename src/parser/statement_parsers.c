#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "util/allocator.h"
#include "util/status.h"

TRY_STATUS decl_statement_parse(Parser* p, DeclStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    DeclStatement* decl_stmt;
    PROPAGATE_IF_ERROR(
        decl_statement_create(p->current_token, NULL, NULL, &decl_stmt, p->allocator.memory_alloc));

    PROPAGATE_IF_ERROR_DO_IS(parser_expect_peek(p, IDENT),
                             decl_statement_destroy((Node*)decl_stmt, p->allocator.free_alloc),
                             UNEXPECTED_TOKEN);
    PROPAGATE_IF_ERROR_DO(identifier_expression_create(p->current_token,
                                                       &decl_stmt->ident,
                                                       p->allocator.memory_alloc,
                                                       p->allocator.free_alloc),
                          decl_statement_destroy((Node*)decl_stmt, p->allocator.free_alloc));

    PROPAGATE_IF_ERROR_DO_IS(parser_expect_peek(p, WALRUS),
                             decl_statement_destroy((Node*)decl_stmt, p->allocator.free_alloc),
                             UNEXPECTED_TOKEN);

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p),
                              decl_statement_destroy((Node*)decl_stmt, p->allocator.free_alloc););
    }

    *stmt = decl_stmt;
    return SUCCESS;
}

TRY_STATUS return_statement_parse(Parser* p, ReturnStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    ReturnStatement* ret_stmt;
    PROPAGATE_IF_ERROR(return_statement_create(NULL, &ret_stmt, p->allocator.memory_alloc));

    PROPAGATE_IF_ERROR_DO(parser_next_token(p),
                          return_statement_destroy((Node*)ret_stmt, p->allocator.free_alloc));

    // TODO: handle skipped expressions to generate value
    while (!parser_current_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p),
                              return_statement_destroy((Node*)ret_stmt, p->allocator.free_alloc));
    }

    *stmt = ret_stmt;
    return SUCCESS;
}

TRY_STATUS expression_statement_parse(Parser* p, ExpressionStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    Expression* lhs;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &lhs));

    ExpressionStatement* expr_stmt;
    PROPAGATE_IF_ERROR_DO(
        expression_statement_create(p->current_token, lhs, &expr_stmt, p->allocator.memory_alloc), {
            Node* lhs_node = (Node*)lhs;
            lhs_node->vtable->destroy(lhs_node, p->allocator.free_alloc);
        });

    if (parser_peek_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), {
            Node* lhs_node = (Node*)lhs;
            lhs_node->vtable->destroy(lhs_node, p->allocator.free_alloc);
        });
    }

    *stmt = expr_stmt;
    return SUCCESS;
}
