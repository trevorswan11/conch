#pragma once

#include <assert.h>
#include <stdbool.h>

#include "lexer/token.h"

#include "parser/parser.h"
#include "parser/precedence.h"

#include "ast/expressions/expression.h"

#include "util/containers/hash_set.h"
#include "util/status.h"

typedef struct TypeExpression TypeExpression;

TRY_STATUS expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression);
TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression);

// Parses a function definition, assuming the current token is a function
TRY_STATUS function_definition_parse(Parser*          p,
                                     ArrayList*       generics,
                                     ArrayList*       parameters,
                                     TypeExpression** return_type,
                                     bool*            contains_default_param);

// Parses an explicit type without considering colon or assignment presence
TRY_STATUS explicit_type_parse(Parser* p, Token start_token, TypeExpression** type);

TRY_STATUS type_expression_parse(Parser* p, Expression** expression, bool* initialized);
TRY_STATUS integer_literal_expression_parse(Parser* p, Expression** expression);
TRY_STATUS float_literal_expression_parse(Parser* p, Expression** expression);
TRY_STATUS prefix_expression_parse(Parser* p, Expression** expression);
TRY_STATUS infix_expression_parse(Parser* p, Expression* left, Expression** expression);
TRY_STATUS bool_expression_parse(Parser* p, Expression** expression);
TRY_STATUS string_expression_parse(Parser* p, Expression** expression);
TRY_STATUS grouped_expression_parse(Parser* p, Expression** expression);
TRY_STATUS if_expression_parse(Parser* p, Expression** expression);
TRY_STATUS function_expression_parse(Parser* p, Expression** expression);
TRY_STATUS call_expression_parse(Parser* p, Expression* function, Expression** expression);
TRY_STATUS struct_expression_parse(Parser* p, Expression** expression);
TRY_STATUS enum_expression_parse(Parser* p, Expression** expression);
TRY_STATUS nil_expression_parse(Parser* p, Expression** expression);
TRY_STATUS continue_expression_parse(Parser* p, Expression** expression);
TRY_STATUS match_expression_parse(Parser* p, Expression** expression);
TRY_STATUS array_literal_expression_parse(Parser* p, Expression** expression);
TRY_STATUS for_loop_expression_parse(Parser* p, Expression** expression);
TRY_STATUS while_loop_expression_parse(Parser* p, Expression** expression);
TRY_STATUS do_while_loop_expression_parse(Parser* p, Expression** expression);

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
    {TYPEOF, &prefix_expression_parse},
    {TRUE, &bool_expression_parse},
    {FALSE, &bool_expression_parse},
    {STRING, &string_expression_parse},
    {MULTILINE_STRING, &string_expression_parse},
    {LPAREN, &grouped_expression_parse},
    {IF, &if_expression_parse},
    {FUNCTION, &function_expression_parse},
    {STRUCT, &struct_expression_parse},
    {ENUM, &enum_expression_parse},
    {NIL, &nil_expression_parse},
    {CONTINUE, &continue_expression_parse},
    {MATCH, &match_expression_parse},
    {LBRACKET, &array_literal_expression_parse},
    {FOR, &for_loop_expression_parse},
    {WHILE, &while_loop_expression_parse},
    {DO, &do_while_loop_expression_parse},
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
    {PLUS, &infix_expression_parse},         {MINUS, &infix_expression_parse},
    {STAR, &infix_expression_parse},         {SLASH, &infix_expression_parse},
    {PERCENT, &infix_expression_parse},      {LT, &infix_expression_parse},
    {LTEQ, &infix_expression_parse},         {GT, &infix_expression_parse},
    {GTEQ, &infix_expression_parse},         {EQ, &infix_expression_parse},
    {NEQ, &infix_expression_parse},          {BOOLEAN_AND, &infix_expression_parse},
    {BOOLEAN_OR, &infix_expression_parse},   {AND, &infix_expression_parse},
    {OR, &infix_expression_parse},           {XOR, &infix_expression_parse},
    {SHR, &infix_expression_parse},          {SHL, &infix_expression_parse},
    {IS, &infix_expression_parse},           {IN, &infix_expression_parse},
    {DOT_DOT, &infix_expression_parse},      {DOT_DOT_EQ, &infix_expression_parse},
    {LPAREN, &call_expression_parse},        {PLUS_ASSIGN, &infix_expression_parse},
    {MINUS_ASSIGN, &infix_expression_parse}, {STAR_ASSIGN, &infix_expression_parse},
    {SLASH_ASSIGN, &infix_expression_parse}, {PERCENT_ASSIGN, &infix_expression_parse},
    {AND_ASSIGN, &infix_expression_parse},   {OR_ASSIGN, &infix_expression_parse},
    {SHL_ASSIGN, &infix_expression_parse},   {SHR_ASSIGN, &infix_expression_parse},
    {NOT_ASSIGN, &infix_expression_parse},   {XOR_ASSIGN, &infix_expression_parse},
    {COMMA, &infix_expression_parse},        {COLON_COLON, &infix_expression_parse},
    {ORELSE, &infix_expression_parse}};
