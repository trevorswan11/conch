#pragma once

#include <assert.h>
#include <stdbool.h>

#include "lexer/token.h"

#include "parser/parser.h"
#include "parser/precedence.h"

#include "ast/expressions/expression.h"

#include "util/containers/hash_set.h"
#include "util/status.h"

TRY_STATUS expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression);
TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression);
TRY_STATUS integer_literal_expression_parse(Parser* p, Expression** expression);
TRY_STATUS float_literal_expression_parse(Parser* p, Expression** expression);
TRY_STATUS prefix_expression_parse(Parser* p, Expression** expression);
TRY_STATUS infix_expression_parse(Parser* p, Expression* left, Expression** expression);

typedef TRY_STATUS (*prefix_parse_fn)(Parser*, Expression**);
typedef TRY_STATUS (*infix_parse_fn)(Parser*, Expression*, Expression**);

typedef struct PrefixFn {
    TokenType       token_key;
    prefix_parse_fn prefix_parse;
} PrefixFn;

static inline Hash hash_prefix(const void* key) {
    assert(sizeof(Hash) == 8);
    const PrefixFn fn = *(const PrefixFn*)key;
    return hash_token_type(&fn.token_key);
}

static inline int compare_prefix(const void* a, const void* b) {
    const PrefixFn fn_a = *(const PrefixFn*)a;
    const PrefixFn fn_b = *(const PrefixFn*)b;
    return (int)fn_a.token_key - (int)fn_b.token_key;
}

static const PrefixFn PREFIX_FUNCTIONS[] = {
    {IDENT, &identifier_expression_parse},
    {INT_2, &integer_literal_expression_parse},
    {INT_8, &integer_literal_expression_parse},
    {INT_10, &integer_literal_expression_parse},
    {INT_16, &integer_literal_expression_parse},
    {UINT_2, &integer_literal_expression_parse},
    {UINT_8, &integer_literal_expression_parse},
    {UINT_10, &integer_literal_expression_parse},
    {UINT_16, &integer_literal_expression_parse},
    {FLOAT, &float_literal_expression_parse},
    {BANG, &prefix_expression_parse},
    {NOT, &prefix_expression_parse},
    {MINUS, &prefix_expression_parse},
};

static inline bool poll_prefix(Parser* p, TokenType type, PrefixFn* prefix) {
    assert(p && prefix);

    SetEntry e;
    PrefixFn prefix_probe = {type, NULL};
    if (STATUS_ERR(hash_set_get_entry(&p->prefix_parse_fns, &prefix_probe, &e))) {
        return false;
    }

    *prefix = *(PrefixFn*)e.key_ptr;
    return true;
}

typedef struct InfixFn {
    TokenType      token_key;
    infix_parse_fn infix_parse;
} InfixFn;

static inline Hash hash_infix(const void* key) {
    assert(sizeof(Hash) == 8);
    const InfixFn fn = *(const InfixFn*)key;
    return hash_token_type(&fn.token_key);
}

static inline int compare_infix(const void* a, const void* b) {
    const InfixFn fn_a = *(const InfixFn*)a;
    const InfixFn fn_b = *(const InfixFn*)b;
    return (int)fn_a.token_key - (int)fn_b.token_key;
}

static inline bool poll_infix(Parser* p, TokenType type, InfixFn* infix) {
    assert(p && infix);

    SetEntry e;
    InfixFn  infix_probe = {type, NULL};
    if (STATUS_ERR(hash_set_get_entry(&p->infix_parse_fns, &infix_probe, &e))) {
        return false;
    }

    *infix = *(InfixFn*)e.key_ptr;
    return true;
}

static const InfixFn INFIX_FUNCTIONS[] = {
    {PLUS, &infix_expression_parse},
    {MINUS, &infix_expression_parse},
    {STAR, &infix_expression_parse},
    {SLASH, &infix_expression_parse},
    {PERCENT, &infix_expression_parse},
    {LT, &infix_expression_parse},
    {LTEQ, &infix_expression_parse},
    {GT, &infix_expression_parse},
    {GTEQ, &infix_expression_parse},
    {EQ, &infix_expression_parse},
    {NEQ, &infix_expression_parse},
    {AND, &infix_expression_parse},
    {OR, &infix_expression_parse},
    {XOR, &infix_expression_parse},
    {SHR, &infix_expression_parse},
    {SHL, &infix_expression_parse},
};
