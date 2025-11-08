#pragma once

#include "lexer/token.h"
#include "util/mem.h"

static const size_t MAX_OPERATOR_LEN = 3;

typedef struct {
    Slice     slice;
    TokenType type;
} Operator;

static const Operator OPERATOR_ASSIGN         = {{"=", 1}, ASSIGN};
static const Operator OPERATOR_WALRUS         = {{":=", 2}, WALRUS};
static const Operator OPERATOR_PLUS           = {{"+", 1}, PLUS};
static const Operator OPERATOR_PLUS_ASSIGN    = {{"+=", 2}, PLUS_ASSIGN};
static const Operator OPERATOR_PLUS_PLUS      = {{"++", 2}, PLUS_PLUS};
static const Operator OPERATOR_MINUS          = {{"-", 1}, MINUS};
static const Operator OPERATOR_MINUS_ASSIGN   = {{"-=", 2}, MINUS_ASSIGN};
static const Operator OPERATOR_MINUS_MINUS    = {{"--", 2}, MINUS_MINUS};
static const Operator OPERATOR_STAR           = {{"*", 1}, STAR};
static const Operator OPERATOR_STAR_ASSIGN    = {{"*=", 2}, STAR_ASSIGN};
static const Operator OPERATOR_STAR_STAR      = {{"**", 2}, STAR_STAR};
static const Operator OPERATOR_SLASH          = {{"/", 1}, SLASH};
static const Operator OPERATOR_SLASH_ASSIGN   = {{"/=", 2}, SLASH_ASSIGN};
static const Operator OPERATOR_PERCENT        = {{"%", 1}, PERCENT};
static const Operator OPERATOR_PERCENT_ASSIGN = {{"%=", 2}, PERCENT_ASSIGN};
static const Operator OPERATOR_BANG           = {{"!", 1}, BANG};
static const Operator OPERATOR_WHAT           = {{"?", 1}, WHAT};

static const Operator OPERATOR_AND        = {{"&", 1}, AND};
static const Operator OPERATOR_AND_ASSIGN = {{"&=", 2}, AND_ASSIGN};
static const Operator OPERATOR_OR         = {{"|", 1}, OR};
static const Operator OPERATOR_OR_ASSIGN  = {{"|=", 2}, OR_ASSIGN};
static const Operator OPERATOR_SHL        = {{"<<", 2}, SHL};
static const Operator OPERATOR_SHL_ASSIGN = {{"<<=", 3}, SHL_ASSIGN};
static const Operator OPERATOR_SHR        = {{">>", 2}, SHR};
static const Operator OPERATOR_SHR_ASSIGN = {{">>=", 3}, SHR_ASSIGN};
static const Operator OPERATOR_NOT        = {{"~", 1}, NOT};
static const Operator OPERATOR_NOT_ASSIGN = {{"~=", 2}, NOT_ASSIGN};
static const Operator OPERATOR_XOR        = {{"^", 1}, XOR};
static const Operator OPERATOR_XOR_ASSIGN = {{"^=", 2}, XOR_ASSIGN};

static const Operator OPERATOR_LT   = {{"<", 1}, LT};
static const Operator OPERATOR_LTEQ = {{"<=", 2}, LTEQ};
static const Operator OPERATOR_GT   = {{">", 1}, GT};
static const Operator OPERATOR_GTEQ = {{">=", 2}, GTEQ};
static const Operator OPERATOR_EQ   = {{"==", 2}, EQ};
static const Operator OPERATOR_NEQ  = {{"!=", 2}, NEQ};

static const Operator OPERATOR_DOT              = {{".", 1}, DOT};
static const Operator OPERATOR_DOT_DOT          = {{"..", 2}, DOT_DOT};
static const Operator OPERATOR_DOT_DOT_EQ       = {{"..=", 3}, DOT_DOT_EQ};
static const Operator OPERATOR_ARROW            = {{"->", 2}, ARROW};
static const Operator OPERATOR_FAT_ARROW        = {{"=>", 2}, FAT_ARROW};
static const Operator OPERATOR_COMMENT          = {{"//", 2}, COMMENT};
static const Operator OPERATOR_MULTILINE_STRING = {{"\\\\", 2}, MULTILINE_STRING};

static const Operator ALL_OPERATORS[] = {
    OPERATOR_ASSIGN,
    OPERATOR_WALRUS,
    OPERATOR_PLUS,
    OPERATOR_PLUS_ASSIGN,
    OPERATOR_PLUS_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MINUS_ASSIGN,
    OPERATOR_MINUS_MINUS,
    OPERATOR_STAR,
    OPERATOR_STAR_ASSIGN,
    OPERATOR_STAR_STAR,
    OPERATOR_SLASH,
    OPERATOR_SLASH_ASSIGN,
    OPERATOR_PERCENT,
    OPERATOR_PERCENT_ASSIGN,
    OPERATOR_BANG,
    OPERATOR_WHAT,

    OPERATOR_AND,
    OPERATOR_AND_ASSIGN,
    OPERATOR_OR,
    OPERATOR_OR_ASSIGN,
    OPERATOR_SHL,
    OPERATOR_SHL_ASSIGN,
    OPERATOR_SHR,
    OPERATOR_SHR_ASSIGN,
    OPERATOR_NOT,
    OPERATOR_NOT_ASSIGN,
    OPERATOR_XOR,
    OPERATOR_XOR_ASSIGN,

    OPERATOR_LT,
    OPERATOR_LTEQ,
    OPERATOR_GT,
    OPERATOR_GTEQ,
    OPERATOR_EQ,
    OPERATOR_NEQ,

    OPERATOR_DOT,
    OPERATOR_DOT_DOT,
    OPERATOR_DOT_DOT_EQ,
    OPERATOR_ARROW,
    OPERATOR_FAT_ARROW,
    OPERATOR_COMMENT,
    OPERATOR_MULTILINE_STRING,
};
