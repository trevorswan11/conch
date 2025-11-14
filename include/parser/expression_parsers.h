#pragma once

#include <assert.h>

#include "lexer/token.h"

#include "parser/parser.h"

#include "ast/expressions/expression.h"

#include "util/containers/hash_set.h"
#include "util/status.h"

typedef TRY_STATUS (*prefix_parse_fn)(Parser*, Expression**);
typedef TRY_STATUS (*infix_parse_fn)(Parser*, Expression*, Expression**);

#define FOREACH_PRECEDENCE(PROCESS)                                                                \
    PROCESS(LOWEST), PROCESS(BOOL_EQUIV), PROCESS(BOOL_LT_GT), PROCESS(ADD_SUB), PROCESS(MUL_DIV), \
        PROCESS(PREFIX), PROCESS(CALL)

typedef enum Precedence {
    FOREACH_PRECEDENCE(GENERATE_ENUM),
} Precedence;

static const char* const PRECEDENCE_NAMES[] = {
    FOREACH_PRECEDENCE(GENERATE_STRING),
};

const char* precedence_name(Precedence precedence);

TRY_STATUS expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression);
TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression);

typedef struct PrefixFn {
    TokenType       token_key;
    prefix_parse_fn prefix_parse;
} PrefixFn;

static inline Hash hash_prefix(const void* key) {
    assert(sizeof(Hash) == 8);
    const PrefixFn fn   = *(const PrefixFn*)key;
    Hash           hash = (Hash)fn.token_key;

    hash = (hash ^ (hash >> 30)) * 0xBF58476D1CE4E5B9ULL;
    hash = (hash ^ (hash >> 27)) * 0x94D049BB133111EBULL;
    hash = hash ^ (hash >> 31);
    return hash;
}

static inline int compare_prefix(const void* a, const void* b) {
    const PrefixFn fn_a = *(const PrefixFn*)a;
    const PrefixFn fn_b = *(const PrefixFn*)b;
    return (int)fn_a.token_key - (int)fn_b.token_key;
}

static const PrefixFn PREFIX_FUNCTIONS[] = {
    {IDENT, &identifier_expression_parse},
};

typedef struct InfixFn {
    TokenType      token_key;
    infix_parse_fn infix_parse;
} InfixFn;

static inline Hash hash_infix(const void* key) {
    assert(sizeof(Hash) == 8);
    const InfixFn fn   = *(const InfixFn*)key;
    Hash          hash = (Hash)fn.token_key;

    hash = (hash ^ (hash >> 30)) * 0xBF58476D1CE4E5B9ULL;
    hash = (hash ^ (hash >> 27)) * 0x94D049BB133111EBULL;
    hash = hash ^ (hash >> 31);
    return hash;
}

static inline int compare_infix(const void* a, const void* b) {
    const InfixFn fn_a = *(const InfixFn*)a;
    const InfixFn fn_b = *(const InfixFn*)b;
    return (int)fn_a.token_key - (int)fn_b.token_key;
}

// static const InfixFn INFIX_FUNCTIONS[] = {};
