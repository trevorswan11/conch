#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "lexer/token.h"

#define FOREACH_PRECEDENCE(PROCESS)                                                                \
    PROCESS(LOWEST), PROCESS(BOOL_EQUIV), PROCESS(BOOL_LT_GT), PROCESS(ADD_SUB), PROCESS(MUL_DIV), \
        PROCESS(EXPONENT), PROCESS(PREFIX), PROCESS(RANGE), PROCESS(ASSIGNMENT),                   \
        PROCESS(NAMESPACE), PROCESS(CALL_IDX)

typedef enum Precedence {
    FOREACH_PRECEDENCE(ENUMERATE),
} Precedence;

static const char* const PRECEDENCE_NAMES[] = {
    FOREACH_PRECEDENCE(STRINGIFY),
};

static inline const char* precedence_name(Precedence precedence) {
    return PRECEDENCE_NAMES[precedence];
}

typedef struct {
    TokenType  token_key;
    Precedence precedence;
} PrecedencePair;

static const PrecedencePair PRECEDENCE_PAIRS[] = {
    {PLUS, ADD_SUB},
    {MINUS, ADD_SUB},
    {STAR, MUL_DIV},
    {SLASH, MUL_DIV},
    {PERCENT, MUL_DIV},
    {STAR_STAR, EXPONENT},
    {LT, BOOL_LT_GT},
    {LTEQ, BOOL_LT_GT},
    {GT, BOOL_LT_GT},
    {GTEQ, BOOL_LT_GT},
    {EQ, BOOL_EQUIV},
    {NEQ, BOOL_EQUIV},
    {BOOLEAN_AND, BOOL_EQUIV},
    {BOOLEAN_OR, BOOL_EQUIV},
    {IS, BOOL_EQUIV},
    {IN, BOOL_EQUIV},
    {AND, ADD_SUB},
    {OR, ADD_SUB},
    {XOR, ADD_SUB},
    {SHR, MUL_DIV},
    {SHL, MUL_DIV},
    {LPAREN, CALL_IDX},
    {LBRACKET, CALL_IDX},
    {DOT_DOT, RANGE},
    {DOT_DOT_EQ, RANGE},
    {ASSIGN, ASSIGNMENT},
    {PLUS_ASSIGN, ASSIGNMENT},
    {MINUS_ASSIGN, ASSIGNMENT},
    {STAR_ASSIGN, ASSIGNMENT},
    {SLASH_ASSIGN, ASSIGNMENT},
    {PERCENT_ASSIGN, ASSIGNMENT},
    {AND_ASSIGN, ASSIGNMENT},
    {OR_ASSIGN, ASSIGNMENT},
    {SHL_ASSIGN, ASSIGNMENT},
    {SHR_ASSIGN, ASSIGNMENT},
    {NOT_ASSIGN, ASSIGNMENT},
    {XOR_ASSIGN, ASSIGNMENT},
    {COLON_COLON, NAMESPACE},
    {ORELSE, ASSIGNMENT},
};

#endif
