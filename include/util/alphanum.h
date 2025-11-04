#pragma once

#include <stdbool.h>

static inline bool is_digit(char byte) {
    return '0' <= byte && byte <= '9';
}

// Checks if the given byte is a letter.
//
// An underscore is considered a letter.
static inline bool is_letter(char byte) {
    bool lower = 'a' <= byte && byte <= 'z';
    bool upper = 'A' <= byte && byte <= 'Z';
    return lower || upper || byte == '_';
}

// Checks if the given byte is a whitespace character.
//
// Includes all common whitespace bytes.
static inline bool is_whitespace(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}
