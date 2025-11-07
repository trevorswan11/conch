#pragma once

#include "lexer/token.h"
#include "util/mem.h"

typedef struct {
    Slice     slice;
    TokenType type;
} Keyword;

static Keyword KEYWORD_FN          = {{"fn", 2}, FUNCTION};
static Keyword KEYWORD_LET         = {{"let", 3}, LET};
static Keyword KEYWORD_STRUCT      = {{"struct", 6}, STRUCT};
static Keyword KEYWORD_TRUE        = {{"true", 4}, TRUE};
static Keyword KEYWORD_FALSE       = {{"false", 5}, FALSE};
static Keyword KEYWORD_BOOLEAN_AND = {{"and", 3}, BOOLEAN_AND};
static Keyword KEYWORD_BOOLEAN_OR  = {{"or", 2}, BOOLEAN_OR};
