#include <stdlib.h>

#include <string.h>

#include "lexer/lexer.h"

Lexer* lexer_create(const char* input) {
    Lexer* l = (Lexer*)malloc(sizeof(Lexer));
    if (!l) {
        return NULL;
    }

    *l = (Lexer){
        .input         = input,
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
    if (l->peek_position >= strlen(l->input)) {
        l->current_byte = '\0';
    } else {
        l->current_byte = l->input[l->peek_position];
    }

    l->position = l->peek_position;
    l->peek_position += 1;
}

Token lexer_next_token(Lexer* l) {
    Token token;

    switch (l->current_byte) {
    case '=':
    case '+':
    case ',':
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
        token = token_init(token_type_from_char(l->current_byte), &l->input[l->position], 1);
        break;
    case '\0':
        token = token_init(token_type_from_char(l->current_byte), &l->input[l->position], 0);
        break;
    default:
        token = token_init(ILLEGAL, "ILLEGAL", strlen("ILLEGAL"));
        break;
    }

    lexer_read_char(l);
    return token;
}
