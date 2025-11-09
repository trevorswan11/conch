#include <assert.h>
#include <stdbool.h>

#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

bool parser_init(Parser* p, Lexer* l, FileIO* io) {
    if (!p || !l) {
        return false;
    }

    *p = (Parser){
        .lexer         = l,
        .lexer_index   = 0,
        .current_token = token_init(END, "", 0),
        .peek_token    = token_init(END, "", 0),
        .io            = io,
    };

    // Read twice to set current and peek
    if (!parser_next_token(p) || !parser_next_token(p)) {
        return false;
    }
    return true;
}

bool parser_consume(Parser* p, AST* ast, FileIO* io) {
    if (!p || !p->lexer || !ast) {
        return false;
    }

    // Reset the lexer and parser to its initial state for potential reuse
    if (!parser_init(p, p->lexer, io)) {
        return false;
    }
    array_list_clear_retaining_capacity(&ast->statements);

    // Traverse the tokens and append until exhausted
    while (p->current_token.type != END) {
        Statement* stmt = parser_parse_statement(p);
        if (stmt) {
            if (!array_list_push(&ast->statements, &stmt)) {
                return false;
            }
        }
        parser_next_token(p);
    }

    return true;
}

bool parser_next_token(Parser* p) {
    p->current_token = p->peek_token;
    return array_list_get(&p->lexer->token_accumulator, p->lexer_index++, &p->peek_token);
}

bool parser_current_token_is(const Parser* p, TokenType t) {
    assert(p);
    return p->current_token.type == t;
}

bool parser_peek_token_is(const Parser* p, TokenType t) {
    assert(p);
    return p->peek_token.type == t;
}

bool parser_expect_peek(Parser* p, TokenType t) {
    assert(p);
    if (parser_peek_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        return false;
    }
}

Statement* parser_parse_statement(Parser* p) {
    switch (p->current_token.type) {
    case VAR: {
        VarStatement* var_stmt = var_statement_parse(p);
        return (Statement*)var_stmt;
    }
    default:
        return NULL;
    }
}
