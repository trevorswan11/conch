#include <stdlib.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "lexer/lexer.h"
#include "util/alphanum.h"
#include "util/mem.h"

Lexer* lexer_create(const char* input) {
    Lexer* l = (Lexer*)malloc(sizeof(Lexer));
    if (!l) {
        return NULL;
    }

    *l = (Lexer){
        .input         = input,
        .input_length  = strlen(input),
        .position      = 0,
        .peek_position = 0,
        .current_byte  = 0,
    };

    lexer_read_char(l);
    return l;
}

void lexer_destroy(Lexer* l) {
    free(l);
    l = NULL;
}

void lexer_read_char(Lexer* l) {
    if (l->peek_position >= l->input_length) {
        l->current_byte = '\0';
    } else {
        l->current_byte = l->input[l->peek_position];
    }

    l->position = l->peek_position;
    l->peek_position += 1;
}

Token lexer_next_token(Lexer* l) {
    lexer_skip_whitespace(l);

    Token      token;
    TokenType  type;
    const bool single_character = token_type_from_char(l->current_byte, &type);

    if (type == END) {
        token = token_init(type, &l->input[l->position], 0);
    } else if (single_character) {
        token = token_init(type, &l->input[l->position], 1);
    } else {
        if (is_letter(l->current_byte)) {
            token.type  = IDENT;
            token.slice = lexer_read_identifier(l);
            return token;
        } else if (is_digit(l->current_byte)) {
            return lexer_read_number(l);
        }

        // Now we can safely say the token is illegal
        token = token_init(ILLEGAL, &l->input[l->position], 1);
    }

    lexer_read_char(l);
    return token;
}

void lexer_skip_whitespace(Lexer* l) {
    while (is_whitespace(l->current_byte)) {
        lexer_read_char(l);
    }
}

Slice lexer_read_identifier(Lexer* l) {
    const size_t start = l->position;
    while (is_letter(l->current_byte)) {
        lexer_read_char(l);
    }

    return (Slice){.ptr = &l->input[start], .length = l->position - start};
}

Token lexer_read_number(Lexer* l) {
    const size_t start          = l->position;
    bool         passed_decimal = false;

    while (is_digit(l->current_byte) || l->current_byte == '.') {
        if (l->current_byte == '.') {
            if (passed_decimal) {
                return token_init(ILLEGAL, &l->input[start], l->position - start);
            }

            passed_decimal = true;
        }

        lexer_read_char(l);
    }

    // Prevent numbers starting and ending with '.'
    if (l->input[start] == '.' || l->input[l->position - 1] == '.') {
        return token_init(ILLEGAL, &l->input[start], l->position - start);
    }

    const TokenType type = passed_decimal ? FLOAT : INT;
    return token_init(type, &l->input[start], l->position - start);
}
