#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "util/memory.h"
#include "util/status.h"

static inline bool is_digit(char byte) { return '0' <= byte && byte <= '9'; }

// Checks if the given byte is a letter.
//
// An underscore is considered a letter.
static inline bool is_letter(char byte) {
    const bool lower = 'a' <= byte && byte <= 'z';
    const bool upper = 'A' <= byte && byte <= 'Z';
    return lower || upper;
}

// Checks if the given byte is a whitespace character.
//
// Includes all common whitespace bytes.
static inline bool is_whitespace(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}

typedef enum {
    UNKNOWN     = 0,
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
} Base;

// Returns the signed integer form of the input string. Case insensitive.
//
// Invariants:
// - The string is at least length n.
// - The characters are valid for the given base.
// - The requested integer is not a negative value.
// - Non-decimal digits have their prefix.
NODISCARD Status strntoll(const char* str, size_t n, Base base, int64_t* value);

// Returns the unsigned integer form of the input string. Case insensitive.
//
// Invariants:
// - The string is at least length n.
// - The characters are valid for the given base.
// - The requested integer is not a negative value.
// - Non-decimal digits have their prefix.
NODISCARD Status strntoull(const char* str, size_t n, Base base, uint64_t* value);

// Returns the double precision floating point form of the input string.
NODISCARD Status strntod(const char*     str,
                         size_t          n,
                         double*         value,
                         memory_alloc_fn memory_alloc,
                         free_alloc_fn   free_alloc);

// Returns the byte representation of a character.
//
// Asserts that the input is surrounded by single quotes and is a single logical ASCII character.
NODISCARD Status strntochr(const char* str, size_t n, uint8_t* out);
