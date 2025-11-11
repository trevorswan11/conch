#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"
#include "util/error.h"
#include "util/io.h"

typedef struct Parser {
    Lexer* lexer;
    size_t lexer_index;
    Token  current_token;
    Token  peek_token;

    ArrayList errors;
    FileIO*   io;
} Parser;

AnyError parser_init(Parser* p, Lexer* l, FileIO* io);

// Deinitializes the parser, freeing only its personally allocated data.
void parser_deinit(Parser* p);

// Consumes all tokens and parses them into an Abstract Syntax Tree.
AnyError parser_consume(Parser* p, AST* ast);
AnyError parser_next_token(Parser* p);

bool     parser_current_token_is(const Parser* p, TokenType t);
bool     parser_peek_token_is(const Parser* p, TokenType t);
AnyError parser_expect_peek(Parser* p, TokenType t);
AnyError parser_peek_error(Parser* p, TokenType t);

AnyError parser_parse_statement(Parser* p, Statement** stmt);
