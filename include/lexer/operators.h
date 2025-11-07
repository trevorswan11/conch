#pragma once

#include "lexer/token.h"
#include "util/mem.h"

typedef struct {
    Slice     slice;
    TokenType type;
} Operator;

static Operator OPERATOR_ASSIGN          = {{"=", 1}, ASSIGN};
static Operator OPERATOR_PLUS            = {{"+", 1}, PLUS};
static Operator OPERATOR_PLUS_ASSIGN     = {{"+=", 2}, PLUS_ASSIGN};
static Operator OPERATOR_MINUS           = {{"-", 1}, MINUS};
static Operator OPERATOR_MINUS_ASSIGN    = {{"-=", 2}, MINUS_ASSIGN};
static Operator OPERATOR_ASTERISK        = {{"*", 1}, ASTERISK};
static Operator OPERATOR_ASTERISK_ASSIGN = {{"*=", 2}, ASTERISK_ASSIGN};
static Operator OPERATOR_SLASH           = {{"/", 1}, SLASH};
static Operator OPERATOR_SLASH_ASSIGN    = {{"/=", 2}, SLASH_ASSIGN};
static Operator OPERATOR_BANG            = {{"!", 1}, BANG};

static Operator OPERATOR_AND        = {{"&", 1}, AND};
static Operator OPERATOR_AND_ASSIGN = {{"&=", 2}, AND_ASSIGN};
static Operator OPERATOR_OR         = {{"|", 1}, OR};
static Operator OPERATOR_OR_ASSIGN  = {{"|=", 2}, OR_ASSIGN};
static Operator OPERATOR_SHL        = {{"<<", 2}, SHL};
static Operator OPERATOR_SHL_ASSIGN = {{"<<=", 3}, SHL_ASSIGN};
static Operator OPERATOR_SHR        = {{">>", 2}, SHR};
static Operator OPERATOR_SHR_ASSIGN = {{">>=", 3}, SHR_ASSIGN};
static Operator OPERATOR_NOT        = {{"~", 1}, NOT};
static Operator OPERATOR_NOT_ASSIGN = {{"~=", 2}, NOT_ASSIGN};

static Operator OPERATOR_LT   = {{"<", 1}, LT};
static Operator OPERATOR_LTEQ = {{"<=", 2}, LTEQ};
static Operator OPERATOR_GT   = {{">", 1}, GT};
static Operator OPERATOR_GTEQ = {{">=", 2}, GTEQ};
static Operator OPERATOR_EQ   = {{"==", 2}, EQ};
static Operator OPERATOR_NEQ  = {{"!=", 2}, NEQ};

static Operator OPERATOR_DOT        = {{".", 1}, DOT};
static Operator OPERATOR_DOT_DOT    = {{"..", 2}, DOT_DOT};
static Operator OPERATOR_DOT_DOT_EQ = {{"..=", 3}, DOT_DOT_EQ};
