#include <stdlib.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lexer/keywords.h"
#include "lexer/lexer.h"
#include "lexer/operators.h"
#include "lexer/token.h"
#include "util/alphanum.h"
#include "util/containers/array_list.h"
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

    const size_t num_keywords = sizeof(ALL_KEYWORDS) / sizeof(ALL_KEYWORDS[0]);
    for (size_t i = 0; i < num_keywords; i++) {
        const Keyword keyword = ALL_KEYWORDS[i];
        hash_map_put_assume_capacity(keyword_map, &keyword.slice, &keyword.type);
    }

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

    const size_t num_operators = sizeof(ALL_OPERATORS) / sizeof(ALL_OPERATORS[0]);
    for (size_t i = 0; i < num_operators; i++) {
        const Operator operator = ALL_OPERATORS[i];
        hash_map_put_assume_capacity(operator_map, &operator.slice, &operator.type);
    }

    return true;
}

Lexer* lexer_create(const char* input) {
    if (!input) {
        return NULL;
    }

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

    ArrayList accumulator;
    if (!array_list_init(&accumulator, 32, sizeof(Token))) {
        free(l);
        hash_map_deinit(&keywords);
        hash_map_deinit(&operators);
        return NULL;
    }

    *l = (Lexer){
        .input             = input,
        .input_length      = strlen(input),
        .position          = 0,
        .peek_position     = 0,
        .current_byte      = 0,
        .token_accumulator = accumulator,
        .keywords          = keywords,
        .operators         = operators,
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
    array_list_deinit(&l->token_accumulator);
    free(l);
    l = NULL;
}

bool lexer_consume(Lexer* l) {
    l->peek_position = 0;
    lexer_read_char(l);
    array_list_clear_retaining_capacity(&l->token_accumulator);

    while (true) {
        const Token token = lexer_next_token(l);
        if (!array_list_push(&l->token_accumulator, &token)) {
            return false;
        }

        if (token.type == END) {
            break;
        }
    }

    return array_list_shrink_to_fit(&l->token_accumulator);
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

void lexer_print_tokens(Lexer* l) {
    assert(l);
    ArrayList*   list        = &l->token_accumulator;
    const size_t accumulated = array_list_length(list);

    Token out;
    for (size_t i = 0; i < accumulated; i++) {
        array_list_get(list, i, &out);
        printf("%s(%.*s)\n", token_type_name(out.type), (int)out.slice.length, out.slice.ptr);
    }
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

static inline bool is_valid_digit(char c, bool is_hex, bool is_bin, bool is_oct) {
    if (is_hex) {
        return (is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
    } else if (is_bin) {
        return (c == '0' || c == '1');
    } else if (is_oct) {
        return (c >= '0' && c <= '7');
    }

    return is_digit(c);
}

Token lexer_read_number(Lexer* l) {
    assert(l->current_byte != '.');

    const size_t start          = l->position;
    bool         passed_decimal = false;
    bool         is_hex = false, is_bin = false, is_oct = false;

    // Detect numeric prefix
    if (l->current_byte == '0' && l->peek_position < l->input_length) {
        char next = l->input[l->peek_position];
        if (next == 'x' || next == 'X') {
            is_hex = true;
            lexer_read_char(l);
            lexer_read_char(l);
        } else if (next == 'b' || next == 'B') {
            is_bin = true;
            lexer_read_char(l);
            lexer_read_char(l);
        } else if (next == 'o' || next == 'O') {
            is_oct = true;
            lexer_read_char(l);
            lexer_read_char(l);
        }
    }

    // Consume digits and handle dot/range rules
    while (is_valid_digit(l->current_byte, is_hex, is_bin, is_oct) ||
           (!is_hex && !is_bin && !is_oct && l->current_byte == '.')) {
        if (l->current_byte == '.') {
            // Stop consuming if we might be entering a dot dot adjacent operator
            if (l->peek_position < l->input_length && l->input[l->peek_position] == '.') {
                break;
            } else if (passed_decimal) {
                break;
            }

            passed_decimal = true;
        }

        lexer_read_char(l);
    }

    // Quick non-base-10 length validation
    if (is_hex || is_bin || is_oct) {
        if (l->position - start <= 2) {
            return token_init(ILLEGAL, &l->input[start], l->position - start);
        }
    }

    // Total validation
    const size_t length = l->position - start;
    TokenType    type   = ILLEGAL;
    if (length == 0)
        return token_init(type, &l->input[start], 1);

    if (l->input[l->position - 1] == '.') {
        return token_init(type, &l->input[start], length);
    }

    if (passed_decimal && (is_hex || is_bin || is_oct)) {
        return token_init(type, &l->input[start], length);
    }

    // Determine the input type
    if (passed_decimal) {
        type = FLOAT;
    } else {
        if (is_hex) {
            type = INT_16;
        } else if (is_bin) {
            type = INT_2;
        } else if (is_oct) {
            type = INT_8;
        } else {
            type = INT_10;
        }
    }

    return token_init(type, &l->input[start], length);
}
