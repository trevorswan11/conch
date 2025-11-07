#pragma once
#include <string.h>

#include "lexer/token.h"
#include "util/mem.h"

typedef struct {
    Slice     slice;
    TokenType type;
} Keyword;

static Keyword KEYWORD_FN  = {{"fn", 2}, FUNCTION};
static Keyword KEYWORD_LET = {{"let", 3}, LET};
