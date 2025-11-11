#pragma once

#include <stdbool.h>

#define TRY_PROPAGATE_ERROR(E) \
    if (E != SUCCESS) {        \
        return E;              \
    }

#define PROPAGATE_IF_ERROR(expr)               \
    do {                                       \
        const AnyError _err_obfuscated = expr; \
        if (_err_obfuscated != SUCCESS) {      \
            return _err_obfuscated;            \
        }                                      \
    } while (0)

#define PROPAGATE_IF_ERROR_DO(expr, action)    \
    do {                                       \
        const AnyError _err_obfuscated = expr; \
        if (_err_obfuscated != SUCCESS) {      \
            action;                            \
            return _err_obfuscated;            \
        }                                      \
    } while (0)

#define PROPAGATE_IF_ERROR_IS(expr, E)         \
    do {                                       \
        const AnyError _err_obfuscated = expr; \
        if (_err_obfuscated == E) {            \
            return _err_obfuscated;            \
        }                                      \
    } while (0)

#define PROPAGATE_IF_ERROR_DO_IS(expr, action, E) \
    do {                                          \
        const AnyError _err_obfuscated = expr;    \
        if (_err_obfuscated == E) {               \
            action;                               \
            return _err_obfuscated;               \
        }                                         \
    } while (0)

#define PROPAGATE_IF_ERROR_NOT(expr, E)        \
    do {                                       \
        const AnyError _err_obfuscated = expr; \
        if (_err_obfuscated != E) {            \
            return _err_obfuscated;            \
        }                                      \
    } while (0)

typedef enum AnyError {
    SUCCESS = 1,
    ALLOCATION_FAILED,
    REALLOCATION_FAILED,
    NULL_PARAMETER,
    VIOLATED_INVARIANT,
    INDEX_OUT_OF_BOUNDS,
    ELEMENT_MISSING,
    ZERO_ITEM_SIZE,
    ZERO_ITEM_ALIGN,
    EMPTY,
    INTEGER_OVERFLOW,
    READ_ERROR,
    TYPE_MISMATCH,
    UNEXPECTED_TOKEN,
} AnyError;
