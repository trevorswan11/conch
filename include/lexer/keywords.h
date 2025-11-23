#pragma once

#include "lexer/token.h"

#include "util/mem.h"

typedef struct {
    Slice     slice;
    TokenType type;
} Keyword;

static const Keyword KEYWORD_FN          = {{"fn", 2}, FUNCTION};
static const Keyword KEYWORD_VAR         = {{"var", 3}, VAR};
static const Keyword KEYWORD_CONST       = {{"const", 5}, CONST};
static const Keyword KEYWORD_STATIC      = {{"static", 6}, STATIC};
static const Keyword KEYWORD_STRUCT      = {{"struct", 6}, STRUCT};
static const Keyword KEYWORD_ENUM        = {{"enum", 4}, ENUM};
static const Keyword KEYWORD_TRUE        = {{"true", 4}, TRUE};
static const Keyword KEYWORD_FALSE       = {{"false", 5}, FALSE};
static const Keyword KEYWORD_BOOLEAN_AND = {{"and", 3}, BOOLEAN_AND};
static const Keyword KEYWORD_BOOLEAN_OR  = {{"or", 2}, BOOLEAN_OR};
static const Keyword KEYWORD_IS          = {{"is", 2}, IS};
static const Keyword KEYWORD_IF          = {{"if", 2}, IF};
static const Keyword KEYWORD_ELSE        = {{"else", 4}, ELSE};
static const Keyword KEYWORD_MATCH       = {{"match", 5}, MATCH};
static const Keyword KEYWORD_CASE        = {{"case", 4}, CASE};
static const Keyword KEYWORD_RETURN      = {{"return", 6}, RETURN};
static const Keyword KEYWORD_FOR         = {{"for", 3}, FOR};
static const Keyword KEYWORD_WHILE       = {{"while", 5}, WHILE};
static const Keyword KEYWORD_CONTINUE    = {{"continue", 8}, CONTINUE};
static const Keyword KEYWORD_BREAK       = {{"break", 5}, BREAK};
static const Keyword KEYWORD_NIL         = {{"nil", 3}, NIL};
static const Keyword KEYWORD_TYPEOF      = {{"typeof", 3}, TYPEOF};
static const Keyword KEYWORD_IMPORT      = {{"import", 3}, IMPORT};
static const Keyword KEYWORD_FROM        = {{"from", 3}, FROM};
static const Keyword KEYWORD_INT         = {{"int", 3}, INT_TYPE};
static const Keyword KEYWORD_UINT        = {{"uint", 4}, UINT_TYPE};
static const Keyword KEYWORD_FLOAT       = {{"float", 5}, FLOAT_TYPE};
static const Keyword KEYWORD_BYTE        = {{"byte", 4}, BYTE_TYPE};
static const Keyword KEYWORD_STRING      = {{"string", 6}, STRING_TYPE};
static const Keyword KEYWORD_BOOL        = {{"bool", 4}, BOOL_TYPE};
static const Keyword KEYWORD_VOID        = {{"void", 4}, VOID_TYPE};
static const Keyword KEYWORD_TYPE        = {{"type", 4}, TYPE_TYPE};

static const Keyword ALL_KEYWORDS[] = {
    KEYWORD_FN,     KEYWORD_VAR,    KEYWORD_CONST,  KEYWORD_STATIC,      KEYWORD_STRUCT,
    KEYWORD_ENUM,   KEYWORD_TRUE,   KEYWORD_FALSE,  KEYWORD_BOOLEAN_AND, KEYWORD_BOOLEAN_OR,
    KEYWORD_IS,     KEYWORD_IF,     KEYWORD_ELSE,   KEYWORD_MATCH,       KEYWORD_CASE,
    KEYWORD_RETURN, KEYWORD_FOR,    KEYWORD_WHILE,  KEYWORD_CONTINUE,    KEYWORD_BREAK,
    KEYWORD_NIL,    KEYWORD_TYPEOF, KEYWORD_IMPORT, KEYWORD_FROM,        KEYWORD_INT,
    KEYWORD_UINT,   KEYWORD_FLOAT,  KEYWORD_BYTE,   KEYWORD_STRING,      KEYWORD_BOOL,
    KEYWORD_VOID,   KEYWORD_TYPE,
};
