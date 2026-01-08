#ifndef PARSER_H
#define PARSER_H

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

[[nodiscard]] Status parser_init(Parser* p, Lexer* l, FileIO* io, Allocator* allocator);
[[nodiscard]] Status parser_null_init(Parser* p, FileIO* io, Allocator* allocator);

// Reinitializes the parser without reallocating internal resources.
[[nodiscard]] Status parser_reset(Parser* p, Lexer* l);

// Gets a pointer to the parser's allocator.
// The parser is asserted to be non-null here.
//
// The returned allocator is guaranteed to have a valid vtable.
Allocator* parser_allocator(Parser* p);

// Deinitializes the parser, freeing only its personally allocated data.
void parser_deinit(Parser* p);

// Consumes all tokens and parses them into an Abstract Syntax Tree.
[[nodiscard]] Status parser_consume(Parser* p, AST* ast);
[[nodiscard]] Status parser_next_token(Parser* p);

bool                 parser_current_token_is(const Parser* p, TokenType t);
bool                 parser_peek_token_is(const Parser* p, TokenType t);
[[nodiscard]] Status parser_expect_current(Parser* p, TokenType t);
[[nodiscard]] Status parser_expect_peek(Parser* p, TokenType t);
[[nodiscard]] Status parser_current_error(Parser* p, TokenType t);
[[nodiscard]] Status parser_peek_error(Parser* p, TokenType t);

Precedence parser_current_precedence(Parser* p);
Precedence parser_peek_precedence(Parser* p);

[[nodiscard]] Status parser_parse_statement(Parser* p, Statement** stmt);

#endif
