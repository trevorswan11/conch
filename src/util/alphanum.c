#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "util/alphanum.h"
#include "util/status.h"

#define PARSE_INT_STR(T, max, overflow_err)             \
    assert(base != UNKNOWN);                            \
    T      result = 0;                                  \
    size_t i      = 0;                                  \
                                                        \
    if (base != DECIMAL) {                              \
        assert(n >= 3);                                 \
        i = 2;                                          \
    }                                                   \
                                                        \
    const T int_base = (T)base;                         \
    while (i < n) {                                     \
        char c = str[i++];                              \
        T    digit;                                     \
                                                        \
        if (c >= '0' && c <= '9') {                     \
            digit = c - '0';                            \
        } else if (c >= 'A' && c <= 'F') {              \
            digit = c - 'A' + 10;                       \
        } else if (c >= 'a' && c <= 'f') {              \
            digit = c - 'a' + 10;                       \
        } else {                                        \
            return MALFORMED_INTEGER_STR;               \
        }                                               \
                                                        \
        if (digit >= int_base) {                        \
            return MALFORMED_INTEGER_STR;               \
        } else if (result > (max - digit) / int_base) { \
            return overflow_err;                        \
        }                                               \
                                                        \
        result = result * (T)int_base + digit;          \
    }                                                   \
                                                        \
    *value = result;                                    \
    return SUCCESS

NODISCARD Status strntoll(const char* str, size_t n, Base base, int64_t* value) {
    PARSE_INT_STR(int64_t, INT64_MAX, SIGNED_INTEGER_OVERFLOW);
}

NODISCARD Status strntoull(const char* str, size_t n, Base base, uint64_t* value) {
    PARSE_INT_STR(uint64_t, UINT64_MAX, UNSIGNED_INTEGER_OVERFLOW);
}

NODISCARD Status strntod(const char*     str,
                         size_t          n,
                         double*         value,
                         memory_alloc_fn memory_alloc,
                         free_alloc_fn   free_alloc) {
    setlocale(LC_NUMERIC, "C");

    assert(memory_alloc && free_alloc);
    char* buf = memory_alloc(n + 1);
    if (!buf) {
        return ALLOCATION_FAILED;
    }

    memcpy(buf, str, n);
    buf[n] = '\0';

    char* endptr;
    errno         = 0;
    double result = strtod(buf, &endptr);

    char end = *endptr;
    free_alloc(buf);

    if (end != '\0') {
        return MALFORMED_FLOAT_STR;
    } else if (errno == ERANGE) {
        return FLOAT_OVERFLOW;
    }

    *value = result;
    return SUCCESS;
}

NODISCARD Status strntochr(const char* str, size_t n, uint8_t* out) {
    if (n != 3 && n != 4) {
        return MALFORMED_CHARATCER_LITERAL;
    }

    if (str[1] != '\\') {
        *out = (uint8_t)str[1];
        return SUCCESS;
    }

    // Check the escaped character and default to a single char
    const uint8_t escaped = str[2];
    switch (escaped) {
    case 'n':
        *out = '\n';
        break;
    case 'r':
        *out = '\r';
        break;
    case 't':
        *out = '\t';
        break;
    case '\\':
        *out = '\\';
        break;
    case '\'':
        *out = '\'';
        break;
    case '"':
        *out = '"';
        break;
    case '0':
        *out = '\0';
        break;
    default:
        *out = (uint8_t)escaped;
        break;
    }

    return SUCCESS;
}
