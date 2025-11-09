#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/statement.h"

#include "util/io.h"

typedef struct Parser {
    Lexer*  lexer;
    size_t  lexer_index;
    Token   current_token;
    Token   peek_token;
    FileIO* io;
} Parser;

bool parser_init(Parser* p, Lexer* l, FileIO* io);

// Consumes all tokens and parses them into an Abstract Syntax Tree.
bool parser_consume(Parser* p, AST* ast, FileIO* io);
bool parser_next_token(Parser* p);

bool parser_current_token_is(const Parser* p, TokenType t);
bool parser_peek_token_is(const Parser* p, TokenType t);
bool parser_expect_peek(Parser* p, TokenType t);

Statement* parser_parse_statement(Parser* p);
