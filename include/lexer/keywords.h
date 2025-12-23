#pragma once

#include "lexer/token.h"

#define KW(str, tok) {{str, sizeof(str) - 1}, tok}

typedef struct {
    Slice     slice;
    TokenType type;
} Keyword;

#define KEYWORD_FN KW("fn", FUNCTION)
#define KEYWORD_VAR KW("var", VAR)
#define KEYWORD_CONST KW("const", CONST)
#define KEYWORD_STRUCT KW("struct", STRUCT)
#define KEYWORD_ENUM KW("enum", ENUM)
#define KEYWORD_TRUE KW("true", TRUE)
#define KEYWORD_FALSE KW("false", FALSE)
#define KEYWORD_BOOLEAN_AND KW("and", BOOLEAN_AND)
#define KEYWORD_BOOLEAN_OR KW("or", BOOLEAN_OR)
#define KEYWORD_IS KW("is", IS)
#define KEYWORD_IN KW("in", IN)
#define KEYWORD_IF KW("if", IF)
#define KEYWORD_ELSE KW("else", ELSE)
#define KEYWORD_ORELSE KW("orelse", ORELSE)
#define KEYWORD_DO KW("do", DO)
#define KEYWORD_MATCH KW("match", MATCH)
#define KEYWORD_RETURN KW("return", RETURN)
#define KEYWORD_FOR KW("for", FOR)
#define KEYWORD_WHILE KW("while", WHILE)
#define KEYWORD_CONTINUE KW("continue", CONTINUE)
#define KEYWORD_BREAK KW("break", BREAK)
#define KEYWORD_NIL KW("nil", NIL)
#define KEYWORD_TYPEOF KW("typeof", TYPEOF)
#define KEYWORD_IMPL KW("impl", IMPL)
#define KEYWORD_IMPORT KW("import", IMPORT)
#define KEYWORD_INT KW("int", INT_TYPE)
#define KEYWORD_UINT KW("uint", UINT_TYPE)
#define KEYWORD_SIZE KW("size", SIZE_TYPE)
#define KEYWORD_FLOAT KW("float", FLOAT_TYPE)
#define KEYWORD_BYTE KW("byte", BYTE_TYPE)
#define KEYWORD_STRING KW("string", STRING_TYPE)
#define KEYWORD_BOOL KW("bool", BOOL_TYPE)
#define KEYWORD_VOID KW("void", VOID_TYPE)
#define KEYWORD_TYPE KW("type", TYPE)
#define KEYWORD_WITH KW("with", WITH)
#define KEYWORD_AS KW("as", AS)

static const Keyword ALL_KEYWORDS[] = {
    KEYWORD_FN,     KEYWORD_VAR,    KEYWORD_CONST,       KEYWORD_STRUCT,     KEYWORD_ENUM,
    KEYWORD_TRUE,   KEYWORD_FALSE,  KEYWORD_BOOLEAN_AND, KEYWORD_BOOLEAN_OR, KEYWORD_IS,
    KEYWORD_IN,     KEYWORD_IF,     KEYWORD_ELSE,        KEYWORD_ORELSE,     KEYWORD_DO,
    KEYWORD_MATCH,  KEYWORD_RETURN, KEYWORD_FOR,         KEYWORD_WHILE,      KEYWORD_CONTINUE,
    KEYWORD_BREAK,  KEYWORD_NIL,    KEYWORD_TYPEOF,      KEYWORD_IMPL,       KEYWORD_IMPORT,
    KEYWORD_INT,    KEYWORD_UINT,   KEYWORD_SIZE,        KEYWORD_FLOAT,      KEYWORD_BYTE,
    KEYWORD_STRING, KEYWORD_BOOL,   KEYWORD_VOID,        KEYWORD_TYPE,       KEYWORD_WITH,
    KEYWORD_AS,
};
