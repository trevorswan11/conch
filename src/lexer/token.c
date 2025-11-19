#include "lexer/token.h"

#include "util/containers/string_builder.h"
#include "util/hash.h"
#include "util/mem.h"

Hash hash_token_type(const void* key) {
    Hash hash = (Hash)(*(const TokenType*)key);
    hash      = (hash ^ (hash >> 30)) * 0xBF58476D1CE4E5B9ULL;
    hash      = (hash ^ (hash >> 27)) * 0x94D049BB133111EBULL;
    hash      = hash ^ (hash >> 31);
    return hash;
}

int compare_token_type(const void* a, const void* b) {
    const TokenType tok_a = *(const TokenType*)a;
    const TokenType tok_b = *(const TokenType*)b;
    return (int)tok_a - (int)tok_b;
}

const char* token_type_name(TokenType type) {
    return TOKEN_TYPE_NAMES[type];
}

bool misc_token_type_from_char(char c, TokenType* t) {
    switch (c) {
    case ',':
        *t = COMMA;
        return true;
    case ':':
        *t = COLON;
        return true;
    case ';':
        *t = SEMICOLON;
        return true;
    case '(':
        *t = LPAREN;
        return true;
    case ')':
        *t = RPAREN;
        return true;
    case '{':
        *t = LBRACE;
        return true;
    case '}':
        *t = RBRACE;
        return true;
    case '[':
        *t = LBRACKET;
        return true;
    case ']':
        *t = RBRACKET;
        return true;
    default:
        return false;
    }
}

bool token_is_integer(TokenType t) {
    return token_is_signed_integer(t) || token_is_unsigned_integer(t);
}

bool token_is_signed_integer(TokenType t) {
    return INT_2 <= t && t <= INT_16;
}

bool token_is_unsigned_integer(TokenType t) {
    return UINT_2 <= t && t <= UINT_16;
}

Token token_init(TokenType t, const char* str, size_t length, size_t line, size_t col) {
    return (Token){
        .type   = t,
        .slice  = slice_from_str_s(str, length),
        .line   = line,
        .column = col,
    };
}

TRY_STATUS promote_token_string(Token token, MutSlice* slice) {
    if (token.type != STRING && token.type != MULTILINE_STRING) {
        return TYPE_MISMATCH;
    }

    StringBuilder builder;
    PROPAGATE_IF_ERROR(string_builder_init(&builder, token.slice.length));

    if (token.type == STRING) {
        if (token.slice.length <= 2) {
            string_builder_deinit(&builder);
            return EMPTY;
        }

        PROPAGATE_IF_ERROR_DO(
            string_builder_append_many(&builder, token.slice.ptr, token.slice.length),
            string_builder_deinit(&builder));
    } else if (token.type == MULTILINE_STRING) {
        if (token.slice.length == 0) {
            string_builder_deinit(&builder);
            return EMPTY;
        }

        bool at_line_start = true;
        for (size_t i = 0; i < token.slice.length; i++) {
            const char c = token.slice.ptr[i];

            // Skip a double backslash at start of line to clean the string
            if (at_line_start) {
                if (c == '\\' && i + 1 < token.slice.length && token.slice.ptr[i + 1] == '\\') {
                    i += 1;
                    continue;
                }
                at_line_start = false;
            }

            PROPAGATE_IF_ERROR_DO(string_builder_append(&builder, c),
                                  string_builder_deinit(&builder));
            if (c == '\n') {
                at_line_start = true;
            }
        }
    }

    PROPAGATE_IF_ERROR_DO(string_builder_to_string(&builder, slice),
                          string_builder_deinit(&builder));
    return SUCCESS;
}
