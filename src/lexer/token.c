#include "lexer/token.h"

#include "util/containers/array_list.h"
#include "util/mem.h"

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

Token token_init(TokenType t, const char* str, size_t length) {
    return (Token){.type = t, .slice = slice_from_s(str, length)};
}

MutSlice promote_token_string(Token token) {
    if (token.type != STRING && token.type != MULTILINE_STRING) {
        return (MutSlice){.ptr = NULL, .length = 0};
    }

    ArrayList buffer;
    if (!array_list_init(&buffer, token.slice.length, sizeof(char))) {
        return (MutSlice){.ptr = NULL, .length = 0};
    }

    if (token.type == STRING) {
        if (token.slice.length <= 2) {
            array_list_deinit(&buffer);
            return (MutSlice){.ptr = NULL, .length = 0};
        }

        for (size_t i = 1; i < token.slice.length - 1; i++) {
            array_list_push_assume_capacity(&buffer, &token.slice.ptr[i]);
        }
    } else if (token.type == MULTILINE_STRING) {
        if (token.slice.length == 0) {
            array_list_deinit(&buffer);
            return (MutSlice){.ptr = NULL, .length = 0};
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

            array_list_push_assume_capacity(&buffer, &c);
            if (c == '\n') {
                at_line_start = true;
            }
        }
    }

    if (!array_list_shrink_to_fit(&buffer)) {
        array_list_deinit(&buffer);
        return (MutSlice){.ptr = NULL, .length = 0};
    }

    return (MutSlice){.ptr = buffer.data, .length = buffer.length};
}
