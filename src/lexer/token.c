#include "lexer/token.h"

bool token_type_from_char(char c, TokenType* t) {
    switch (c) {
    case '=':
        *t = ASSIGN;
        return true;
    case '+':
        *t = PLUS;
        return true;
    case ',':
        *t = COMMA;
        return true;
    case ';':
        *t = SEMICOLON;
        return true;
    case '(':
        *t = LPAREN;
        return true;
    case ')':
        *t = RPAREN;
        return true;
    case '{':
        *t = LBRACE;
        return true;
    case '}':
        *t = RBRACE;
        return true;
    case '\0':
        *t = END;
        return true;
    default:
        *t = ILLEGAL;
        return false;
    }
}
