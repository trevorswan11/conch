#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "parser/precedence.h"

#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/hash_set.h"
#include "util/io.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct Parser {
    Lexer* lexer;
    size_t lexer_index;
    Token  current_token;
    Token  peek_token;

    HashSet prefix_parse_fns;
    HashSet infix_parse_fns;
    HashMap precedences;

    ArrayList errors;
    FileIO*   io;

    Allocator allocator;
} Parser;

TRY_STATUS parser_init(Parser* p, Lexer* l, FileIO* io, Allocator allocator);

// Deinitializes the parser, freeing only its personally allocated data.
void parser_deinit(Parser* p);

// Consumes all tokens and parses them into an Abstract Syntax Tree.
TRY_STATUS parser_consume(Parser* p, AST* ast);
TRY_STATUS parser_next_token(Parser* p);

bool       parser_current_token_is(const Parser* p, TokenType t);
bool       parser_peek_token_is(const Parser* p, TokenType t);
TRY_STATUS parser_expect_peek(Parser* p, TokenType t);
TRY_STATUS parser_peek_error(Parser* p, TokenType t);

Precedence parser_current_precedence(Parser* p);
Precedence parser_peek_precedence(Parser* p);

TRY_STATUS parser_parse_statement(Parser* p, Statement** stmt);

// Adds an error message to the parser detailing an error from a status code.
TRY_STATUS parser_put_status_error(Parser* p, Status status, size_t line, size_t col);

// Appends " [Ln <>, Col <>]" to the given builder.
TRY_STATUS error_append_ln_col(size_t line, size_t col, StringBuilder* sb);
