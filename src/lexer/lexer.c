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
#include "util/io.h"
#include "util/mem.h"
#include "util/status.h"

static inline TRY_STATUS _init_keywords(HashMap* keyword_map, Allocator allocator) {
    PROPAGATE_IF_ERROR(hash_map_init_allocator(keyword_map,
                                               32,
                                               sizeof(Slice),
                                               alignof(Slice),
                                               sizeof(TokenType),
                                               alignof(TokenType),
                                               hash_slice,
                                               compare_slices,
                                               allocator));

    const size_t num_keywords = sizeof(ALL_KEYWORDS) / sizeof(ALL_KEYWORDS[0]);
    for (size_t i = 0; i < num_keywords; i++) {
        const Keyword keyword = ALL_KEYWORDS[i];
        hash_map_put_assume_capacity(keyword_map, &keyword.slice, &keyword.type);
    }

    return SUCCESS;
}

static inline TRY_STATUS _init_operators(HashMap* operator_map, Allocator allocator) {
    PROPAGATE_IF_ERROR(hash_map_init_allocator(operator_map,
                                               64,
                                               sizeof(Slice),
                                               alignof(Slice),
                                               sizeof(TokenType),
                                               alignof(TokenType),
                                               hash_slice,
                                               compare_slices,
                                               allocator));

    const size_t num_operators = sizeof(ALL_OPERATORS) / sizeof(ALL_OPERATORS[0]);
    for (size_t i = 0; i < num_operators; i++) {
        const Operator op = ALL_OPERATORS[i];
        hash_map_put_assume_capacity(operator_map, &op.slice, &op.type);
    }

    return SUCCESS;
}

TRY_STATUS lexer_init(Lexer* l, const char* input, Allocator allocator) {
    if (!input) {
        return NULL_PARAMETER;
    }
    PROPAGATE_IF_ERROR(lexer_null_init(l, allocator));

    l->input        = input;
    l->input_length = strlen(input);

    lexer_read_char(l);
    return SUCCESS;
}

TRY_STATUS lexer_null_init(Lexer* l, Allocator allocator) {
    if (!l) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(allocator);

    HashMap keywords;
    PROPAGATE_IF_ERROR(_init_keywords(&keywords, allocator));

    HashMap operators;
    PROPAGATE_IF_ERROR_DO(_init_operators(&operators, allocator), hash_map_deinit(&keywords));

    ArrayList accumulator;
    PROPAGATE_IF_ERROR_DO(array_list_init(&accumulator, 32, sizeof(Token)), {
        hash_map_deinit(&keywords);
        hash_map_deinit(&operators);
    });

    *l = (Lexer){
        .input             = NULL,
        .input_length      = 0,
        .position          = 0,
        .peek_position     = 0,
        .current_byte      = 0,
        .token_accumulator = accumulator,
        .line_no           = 1,
        .col_no            = 0,
        .keywords          = keywords,
        .operators         = operators,
        .allocator         = allocator,
    };

    return SUCCESS;
}

void lexer_deinit(Lexer* l) {
    if (!l) {
        return;
    }

    hash_map_deinit(&l->keywords);
    hash_map_deinit(&l->operators);
    array_list_deinit(&l->token_accumulator);
}

TRY_STATUS lexer_consume(Lexer* l) {
    l->peek_position = 0;
    l->line_no       = 1;
    l->col_no        = 0;

    lexer_read_char(l);
    array_list_clear_retaining_capacity(&l->token_accumulator);

    while (true) {
        const Token token = lexer_next_token(l);
        PROPAGATE_IF_ERROR(array_list_push(&l->token_accumulator, &token));

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

    if (l->current_byte == '\n') {
        l->line_no += 1;
        l->col_no = 0;
    } else {
        l->col_no += 1;
    }

    l->position = l->peek_position;
    l->peek_position += 1;
}

Token lexer_next_token(Lexer* l) {
    lexer_skip_whitespace(l);

    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;

    Token token;
    token.line                 = start_line;
    token.column               = start_col;
    const Token maybe_operator = lexer_read_operator(l);

    if (maybe_operator.type != ILLEGAL) {
        if (maybe_operator.type == END) {
            return maybe_operator;
        }

        for (size_t i = 0; i < maybe_operator.slice.length; ++i) {
            lexer_read_char(l);
        }

        if (maybe_operator.type == COMMENT) {
            return lexer_read_comment(l);
        } else if (maybe_operator.type == MULTILINE_STRING) {
            return lexer_read_multilinestring(l);
        } else {
            return maybe_operator;
        }
    } else {
        if (misc_token_type_from_char(l->current_byte, &token.type)) {
            token.slice = slice_from_str_s(&l->input[l->position], 1);
        } else if (is_letter(l->current_byte)) {
            token.slice = lexer_read_identifier(l);
            token.type  = lexer_lookup_identifier(l, &token.slice);
            return token;
        } else if (is_digit(l->current_byte)) {
            return lexer_read_number(l);
        } else if (l->current_byte == '"') {
            return lexer_read_string(l);
        } else if (l->current_byte == '\'') {
            return lexer_read_character_literal(l);
        } else {
            token = token_init(ILLEGAL, &l->input[l->position], 1, start_line, start_col);
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
    if (STATUS_ERR(hash_map_get_value(&l->keywords, literal, &value))) {
        return IDENT;
    }
    return value;
}

TRY_STATUS lexer_print_tokens(FileIO* io, Lexer* l) {
    assert(l);
    ArrayList*   list        = &l->token_accumulator;
    const size_t accumulated = array_list_length(list);

    Token out;
    for (size_t i = 0; i < accumulated; i++) {
        PROPAGATE_IF_ERROR(array_list_get(list, i, &out));
        PROPAGATE_IF_IO_ERROR(fprintf(io->out,
                                      "%s(%.*s) [%zu, %zu]\n",
                                      token_type_name(out.type),
                                      (int)out.slice.length,
                                      out.slice.ptr,
                                      out.line,
                                      out.column));
    }
    return SUCCESS;
}

// Tries to read an operator or EOF from the position, returning ILLEGAL otherwise.
Token lexer_read_operator(Lexer* l) {
    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;

    if (l->current_byte == '\0') {
        return token_init(END, &l->input[l->position], 0, start_line, start_col);
    }

    size_t    max_len      = 0;
    TokenType matched_type = ILLEGAL;

    // Try extending from length 1 up to the max operator size
    for (size_t len = 1; len <= MAX_OPERATOR_LEN && l->position + len <= l->input_length; len++) {
        Slice query = slice_from_str_s(&l->input[l->position], len);
        if (STATUS_OK(hash_map_get_value(&l->operators, &query, &matched_type))) {
            max_len = len;
        }
    }

    // We cannot greedily consume the lexer here since the next token instruction handles that
    if (max_len == 0) {
        return token_init(ILLEGAL, &l->input[l->position], 1, start_line, start_col);
    }
    return token_init(matched_type, &l->input[l->position], max_len, start_line, start_col);
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
    const size_t start_line     = l->line_no;
    const size_t start_col      = l->col_no;
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
            return token_init(
                ILLEGAL, &l->input[start], l->position - start, start_line, start_col);
        }
    }

    // Total validation
    const size_t length = l->position - start;
    TokenType    type   = ILLEGAL;
    if (length == 0)
        return token_init(type, &l->input[start], 1, start_line, start_col);

    if (l->input[l->position - 1] == '.') {
        return token_init(type, &l->input[start], length, start_line, start_col);
    }

    if (passed_decimal && (is_hex || is_bin || is_oct)) {
        return token_init(type, &l->input[start], length, start_line, start_col);
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

    return token_init(type, &l->input[start], length, start_line, start_col);
}

static inline char lexer_read_escape(Lexer* l) {
    lexer_read_char(l);

    switch (l->current_byte) {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case '\\':
        return '\\';
    case '\'':
        return '\'';
    case '"':
        return '"';
    case '0':
        return '\0';
    default:
        return l->current_byte;
    }
}

Token lexer_read_string(Lexer* l) {
    const size_t start      = l->position;
    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;
    lexer_read_char(l);

    while (l->current_byte != '"' && l->current_byte != '\0') {
        if (l->current_byte == '\\') {
            lexer_read_escape(l);
        }
        lexer_read_char(l);
    }

    if (l->current_byte == '\0') {
        return token_init(ILLEGAL, &l->input[start], l->position - start, start_line, start_col);
    }
    lexer_read_char(l);

    return token_init(STRING, &l->input[start], l->position - start, start_line, start_col);
}

// Reads a character literal returning an illegal token for malformed literals.
//
// Assumes that the surrounding single quotes have not been consumed.
Token lexer_read_character_literal(Lexer* l) {
    const size_t start      = l->position;
    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;
    lexer_read_char(l);

    // Consume one logical character
    if (l->current_byte == '\\') {
        lexer_read_escape(l);
        lexer_read_char(l);
    } else if (l->current_byte != '\'' && l->current_byte != '\n' && l->current_byte != '\r') {
        lexer_read_char(l);
    } else {
        return token_init(ILLEGAL, &l->input[start], l->position - start, start_line, start_col);
    }

    // The next character MUST be closing ', otherwise illegally consume like a comment
    if (l->current_byte != '\'') {
        size_t illegal_end = l->position;
        while (l->current_byte != '\'' && l->current_byte != '\n' && l->current_byte != '\r' &&
               l->current_byte != '\0') {
            lexer_read_char(l);
            illegal_end = l->position;
        }

        if (l->current_byte == '\'') {
            lexer_read_char(l);
            illegal_end = l->position;
        }

        return token_init(ILLEGAL, &l->input[start], illegal_end - start, start_line, start_col);
    }
    lexer_read_char(l);

    return token_init(CHARACTER, &l->input[start], l->position - start, start_line, start_col);
}

// Reads a multiline string from the token, assuming the '//' operator has been consumed
Token lexer_read_comment(Lexer* l) {
    const size_t start      = l->position;
    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;
    while (l->current_byte != '\n' && l->current_byte != '\0') {
        lexer_read_char(l);
    }
    return token_init(COMMENT, &l->input[start], l->position - start, start_line, start_col);
}

// Reads a multiline string from the token, assuming the '\\' operator has been consumed
Token lexer_read_multilinestring(Lexer* l) {
    const size_t start      = l->position;
    const size_t start_line = l->line_no;
    const size_t start_col  = l->col_no;
    size_t       end        = start;

    while (true) {
        // Consume characters until newline or EOF
        while (l->current_byte != '\n' && l->current_byte != '\r' && l->current_byte != '\0') {
            lexer_read_char(l);
        }

        // Peek positions
        size_t peek_pos = l->peek_position;
        if (l->current_byte == '\r' && peek_pos < l->input_length && l->input[peek_pos] == '\n') {
            peek_pos += 1;
        }

        bool has_continuation = false;
        if ((l->current_byte == '\n' || l->current_byte == '\r') &&
            peek_pos + 1 < l->input_length && l->input[peek_pos] == '\\' &&
            l->input[peek_pos + 1] == '\\') {
            has_continuation = true;
        }

        // Don't include the newline if there is no continuation to prevent trailing whitespace
        if (!has_continuation) {
            end = l->position;
            break;
        }

        // Include the CRLF/LF newline in the token
        lexer_read_char(l);
        if (l->current_byte == '\r' && l->peek_position < l->input_length &&
            l->input[l->peek_position] == '\n') {
            lexer_read_char(l);
        }

        // consume the next "\\" line continuation
        lexer_read_char(l);
        lexer_read_char(l);
    }

    return token_init(MULTILINE_STRING, &l->input[start], end - start, start_line, start_col);
}
