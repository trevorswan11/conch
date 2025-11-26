#pragma once

#include "lexer/token.h"

#include "util/status.h"

#define FOREACH_PRECEDENCE(PROCESS)                                                                \
    PROCESS(LOWEST), PROCESS(BOOL_EQUIV), PROCESS(BOOL_LT_GT), PROCESS(ADD_SUB), PROCESS(MUL_DIV), \
        PROCESS(PREFIX), PROCESS(CALL)

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
    {LT, BOOL_LT_GT},
    {LTEQ, BOOL_LT_GT},
    {GT, BOOL_LT_GT},
    {GTEQ, BOOL_LT_GT},
    {EQ, BOOL_EQUIV},
    {NEQ, BOOL_EQUIV},
    {AND, ADD_SUB},
    {OR, ADD_SUB},
    {XOR, ADD_SUB},
    {SHR, MUL_DIV},
    {SHL, MUL_DIV},
    {LPAREN, CALL},
};
