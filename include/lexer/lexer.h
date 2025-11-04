#pragma once

#include <stdint.h>

#include "token/token.h"

// A lexer object which does not own the string input.
typedef struct {
    const char* input;
    size_t      position;
    size_t      peek_position;
    char        current_byte;
} Lexer;

Lexer* lexer_create(const char* input);
void   lexer_destroy(Lexer* l);

void  lexer_read_char(Lexer* l);
Token lexer_next_token(Lexer* l);
