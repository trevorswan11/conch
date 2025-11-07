#include <stdlib.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "lexer/keywords.h"
#include "lexer/lexer.h"
#include "lexer/operators.h"
#include "lexer/token.h"
#include "util/alphanum.h"
#include "util/containers/hash_map.h"
#include "util/hash.h"
#include "util/mem.h"

static inline bool _init_keywords(HashMap* keyword_map) {
    if (!keyword_map || !hash_map_init(keyword_map,
                                       32,
                                       sizeof(Slice),
                                       alignof(Slice),
                                       sizeof(TokenType),
                                       alignof(TokenType),
                                       hash_slice,
                                       compare_slices)) {
        return false;
    }

    hash_map_put_assume_capacity(keyword_map, &KEYWORD_FN.slice, &KEYWORD_FN.type);
    hash_map_put_assume_capacity(keyword_map, &KEYWORD_LET.slice, &KEYWORD_LET.type);
    hash_map_put_assume_capacity(keyword_map, &KEYWORD_STRUCT.slice, &KEYWORD_STRUCT.type);
    hash_map_put_assume_capacity(keyword_map, &KEYWORD_TRUE.slice, &KEYWORD_TRUE.type);
    hash_map_put_assume_capacity(keyword_map, &KEYWORD_FALSE.slice, &KEYWORD_FALSE.type);
    hash_map_put_assume_capacity(
        keyword_map, &KEYWORD_BOOLEAN_AND.slice, &KEYWORD_BOOLEAN_AND.type);
    hash_map_put_assume_capacity(keyword_map, &KEYWORD_BOOLEAN_OR.slice, &KEYWORD_BOOLEAN_OR.type);

    return true;
}

static inline bool _init_operators(HashMap* operator_map) {
    if (!operator_map || !hash_map_init(operator_map,
                                        64,
                                        sizeof(Slice),
                                        alignof(Slice),
                                        sizeof(TokenType),
                                        alignof(TokenType),
                                        hash_slice,
                                        compare_slices)) {
        return false;
    }

    // Arithmetic operators
    hash_map_put_assume_capacity(operator_map, &OPERATOR_ASSIGN.slice, &OPERATOR_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_PLUS.slice, &OPERATOR_PLUS.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_PLUS_ASSIGN.slice, &OPERATOR_PLUS_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_MINUS.slice, &OPERATOR_MINUS.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_MINUS_ASSIGN.slice, &OPERATOR_MINUS_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_ASTERISK.slice, &OPERATOR_ASTERISK.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_ASTERISK_ASSIGN.slice, &OPERATOR_ASTERISK_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_SLASH.slice, &OPERATOR_SLASH.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_SLASH_ASSIGN.slice, &OPERATOR_SLASH_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_BANG.slice, &OPERATOR_BANG.type);

    // Bitwise operators
    hash_map_put_assume_capacity(operator_map, &OPERATOR_AND.slice, &OPERATOR_AND.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_AND_ASSIGN.slice, &OPERATOR_AND_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_OR.slice, &OPERATOR_OR.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_OR_ASSIGN.slice, &OPERATOR_OR_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_SHL.slice, &OPERATOR_SHL.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_SHL_ASSIGN.slice, &OPERATOR_SHL_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_SHR.slice, &OPERATOR_SHR.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_SHR_ASSIGN.slice, &OPERATOR_SHR_ASSIGN.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_NOT.slice, &OPERATOR_NOT.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_NOT_ASSIGN.slice, &OPERATOR_NOT_ASSIGN.type);

    // Boolean operators
    hash_map_put_assume_capacity(operator_map, &OPERATOR_LT.slice, &OPERATOR_LT.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_LTEQ.slice, &OPERATOR_LTEQ.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_GT.slice, &OPERATOR_GT.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_GTEQ.slice, &OPERATOR_GTEQ.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_EQ.slice, &OPERATOR_EQ.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_NEQ.slice, &OPERATOR_NEQ.type);

    // Other operators
    hash_map_put_assume_capacity(operator_map, &OPERATOR_DOT.slice, &OPERATOR_DOT.type);
    hash_map_put_assume_capacity(operator_map, &OPERATOR_DOT_DOT.slice, &OPERATOR_DOT_DOT.type);
    hash_map_put_assume_capacity(
        operator_map, &OPERATOR_DOT_DOT_EQ.slice, &OPERATOR_DOT_DOT_EQ.type);

    return true;
}

Lexer* lexer_create(const char* input) {
    Lexer* l = (Lexer*)malloc(sizeof(Lexer));
    if (!l) {
        return NULL;
    }

    HashMap keywords;
    if (!_init_keywords(&keywords)) {
        free(l);
        return NULL;
    }

    HashMap operators;
    if (!_init_operators(&operators)) {
        free(l);
        hash_map_deinit(&keywords);
        return NULL;
    }

    *l = (Lexer){
        .input         = input,
        .input_length  = strlen(input),
        .position      = 0,
        .peek_position = 0,
        .current_byte  = 0,
        .keywords      = keywords,
        .operators     = operators,
    };

    lexer_read_char(l);
    return l;
}

void lexer_destroy(Lexer* l) {
    if (!l) {
        return;
    }

    hash_map_deinit(&l->keywords);
    hash_map_deinit(&l->operators);
    free(l);
    l = NULL;
}

void lexer_read_char(Lexer* l) {
    if (l->peek_position >= l->input_length) {
        l->current_byte = '\0';
    } else {
        l->current_byte = l->input[l->peek_position];
    }

    l->position = l->peek_position;
    l->peek_position += 1;
}

Token lexer_next_token(Lexer* l) {
    lexer_skip_whitespace(l);

    Token       token;
    const Token maybe_operator = lexer_read_operator(l);

    if (maybe_operator.type != ILLEGAL) {
        if (maybe_operator.type == END) {
            return maybe_operator;
        }

        for (size_t i = 0; i < maybe_operator.slice.length; ++i) {
            lexer_read_char(l);
        }
        return maybe_operator;
    } else {
        if (misc_token_type_from_char(l->current_byte, &token.type)) {
            token.slice = slice_from_s(&l->input[l->position], 1);
        } else if (is_letter(l->current_byte)) {
            token.slice = lexer_read_identifier(l);
            token.type  = lexer_lookup_identifier(l, &token.slice);
            return token;
        } else if (is_digit(l->current_byte)) {
            return lexer_read_number(l);
        } else {
            token = token_init(ILLEGAL, &l->input[l->position], 1);
        }
    }

    lexer_read_char(l);
    return token;
}

void lexer_skip_whitespace(Lexer* l) {
    while (is_whitespace(l->current_byte)) {
        lexer_read_char(l);
    }
}

TokenType lexer_lookup_identifier(Lexer* l, const Slice* literal) {
    TokenType value;
    if (!hash_map_get_value(&l->keywords, literal, &value)) {
        return IDENT;
    }
    return value;
}

Token lexer_read_operator(Lexer* l) {
    if (l->current_byte == '\0') {
        return token_init(END, &l->input[l->position], 0);
    }

    size_t    max_len      = 0;
    TokenType matched_type = ILLEGAL;

    // Try extending from length 1 up to the max operator size
    for (size_t len = 1; len <= MAX_OPERATOR_LEN && l->position + len <= l->input_length; len++) {
        Slice query = slice_from_s(&l->input[l->position], len);
        if (hash_map_get_value(&l->operators, &query, &matched_type)) {
            max_len = len;
        }
    }

    // We cannot greedily consume the lexer here since the next token instruction handles that
    if (max_len == 0) {
        return token_init(ILLEGAL, &l->input[l->position], 1);
    }
    return token_init(matched_type, &l->input[l->position], max_len);
}

Slice lexer_read_identifier(Lexer* l) {
    const size_t start = l->position;
    while (is_letter(l->current_byte)) {
        lexer_read_char(l);
    }

    return (Slice){.ptr = &l->input[start], .length = l->position - start};
}

Token lexer_read_number(Lexer* l) {
    const size_t start          = l->position;
    bool         passed_decimal = false;

    while (is_digit(l->current_byte) || l->current_byte == '.') {
        if (l->current_byte == '.') {
            if (passed_decimal) {
                return token_init(ILLEGAL, &l->input[start], l->position - start);
            }

            passed_decimal = true;
        }

        lexer_read_char(l);
    }

    // Prevent numbers starting and ending with '.'
    if (l->input[start] == '.' || l->input[l->position - 1] == '.') {
        return token_init(ILLEGAL, &l->input[start], l->position - start);
    }

    const TokenType type = passed_decimal ? FLOAT : INT;
    return token_init(type, &l->input[start], l->position - start);
}
