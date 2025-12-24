#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "lexer/token.h"

#include "parser/precedence.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/hash_set.h"
#include "util/io.h"
#include "util/status.h"

typedef struct Lexer     Lexer;
typedef struct AST       AST;
typedef struct Statement Statement;

typedef struct Parser {
    Lexer* lexer;
    size_t lexer_index;
    Token  current_token;
    Token  peek_token;

    HashSet prefix_parse_fns;
    HashSet infix_parse_fns;
    HashSet primitives;
    HashMap precedences;

    ArrayList errors;
    FileIO*   io;

    Allocator allocator;
} Parser;

NODISCARD Status parser_init(Parser* p, Lexer* l, FileIO* io, Allocator allocator);
NODISCARD Status parser_null_init(Parser* p, FileIO* io, Allocator allocator);

// Reinitializes the parser without reallocating internal resources.
NODISCARD Status parser_reset(Parser* p, Lexer* l);

// Deinitializes the parser, freeing only its personally allocated data.
void parser_deinit(Parser* p);

// Consumes all tokens and parses them into an Abstract Syntax Tree.
NODISCARD Status parser_consume(Parser* p, AST* ast);
NODISCARD Status parser_next_token(Parser* p);

bool             parser_current_token_is(const Parser* p, TokenType t);
bool             parser_peek_token_is(const Parser* p, TokenType t);
NODISCARD Status parser_expect_current(Parser* p, TokenType t);
NODISCARD Status parser_expect_peek(Parser* p, TokenType t);
NODISCARD Status parser_current_error(Parser* p, TokenType t);
NODISCARD Status parser_peek_error(Parser* p, TokenType t);

Precedence parser_current_precedence(Parser* p);
Precedence parser_peek_precedence(Parser* p);

NODISCARD Status parser_parse_statement(Parser* p, Statement** stmt);

#endif
