#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lexer/token.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/mem.h"

// A lexer object which does not own the string input.
typedef struct {
    const char* input;
    size_t      input_length;
    size_t      position;
    size_t      peek_position;
    char        current_byte;
    ArrayList   token_accumulator;

    HashMap keywords;
    HashMap operators;
} Lexer;

Lexer* lexer_create(const char* input);
void   lexer_destroy(Lexer* l);

// Consumes all tokens in the input and saves them to the internal token list.
//
// Will always refresh the internal list and start position when called.
bool      lexer_consume(Lexer* l);
void      lexer_read_char(Lexer* l);
Token     lexer_next_token(Lexer* l);
void      lexer_skip_whitespace(Lexer* l);
TokenType lexer_lookup_identifier(Lexer* l, const Slice* literal);
void      lexer_print_tokens(Lexer* l);

Token lexer_read_operator(Lexer* l);
Slice lexer_read_identifier(Lexer* l);
Token lexer_read_number(Lexer* l);
Token lexer_read_string(Lexer* l);
Token lexer_read_character_literal(Lexer* l);
Token lexer_read_comment(Lexer* l);
Token lexer_read_multilinestring(Lexer* l);
