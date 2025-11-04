#pragma once

#include <stdbool.h>

static inline bool is_digit(char byte) {
    return '0' <= byte && byte <= '9';
}

static inline bool is_letter(char byte) {
    bool lower = 'a' <= byte && byte <= 'z';
    bool upper = 'A' <= byte && byte <= 'Z';
    return lower || upper || byte == '_';
}

static inline bool is_whitespace(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}
