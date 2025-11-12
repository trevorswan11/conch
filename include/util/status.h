#pragma once

#include <stdbool.h>

#define GENERATE_ENUM(ENUM) ENUM
#define GENERATE_STRING(STRING) #STRING

#define TRY_PROPAGATE_ERROR(E) \
    if (E != SUCCESS) {        \
        return E;              \
    }

#define PROPAGATE_IF_ERROR(expr)              \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        if (_stat_obfuscated != SUCCESS) {    \
            return _stat_obfuscated;          \
        }                                     \
    } while (0)

#define PROPAGATE_IF_ERROR_DO(expr, action)   \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        if (_stat_obfuscated != SUCCESS) {    \
            action;                           \
            return _stat_obfuscated;          \
        }                                     \
    } while (0)

#define PROPAGATE_IF_ERROR_IS(expr, S)        \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        if (_stat_obfuscated == S) {          \
            return _stat_obfuscated;          \
        }                                     \
    } while (0)

#define PROPAGATE_IF_ERROR_DO_IS(expr, action, S) \
    do {                                          \
        const Status _stat_obfuscated = expr;     \
        if (_stat_obfuscated == S) {              \
            action;                               \
            return _stat_obfuscated;              \
        }                                         \
    } while (0)

#define PROPAGATE_IF_ERROR_NOT(expr, S)       \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        if (_stat_obfuscated != S) {          \
            return _stat_obfuscated;          \
        }                                     \
    } while (0)

// Allows a variable to be left unused.
//
// For unused error codes, see `UNREACHABLE_ERROR` or `IGNORE_STATUS`.
#define MAYBE_UNUSED(x) ((void)(x))

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE_IMPL __builtin_unreachable()
#else
#define UNREACHABLE_IMPL abort()
#endif

// Prints to stderr without considering the chance of IO failure.
void debug_print(const char* format, ...);

#define UNREACHABLE_ERROR(expr)                                                           \
    do {                                                                                  \
        const Status _stat_obfuscated = expr;                                             \
        if (_stat_obfuscated != SUCCESS) {                                                \
            debug_print("Panic: reached unreachable code (%s:%d)\n", __FILE__, __LINE__); \
            UNREACHABLE_IMPL;                                                             \
        }                                                                                 \
    } while (0)

#define FOREACH_STATUS(PROCESS)                                                               \
    PROCESS(SUCCESS), PROCESS(ALLOCATION_FAILED), PROCESS(REALLOCATION_FAILED),               \
        PROCESS(NULL_PARAMETER), PROCESS(VIOLATED_INVARIANT), PROCESS(INDEX_OUT_OF_BOUNDS),   \
        PROCESS(ELEMENT_MISSING), PROCESS(ZERO_ITEM_SIZE), PROCESS(ZERO_ITEM_ALIGN),          \
        PROCESS(EMPTY), PROCESS(INTEGER_OVERFLOW), PROCESS(READ_ERROR), PROCESS(WRITE_ERROR), \
        PROCESS(TYPE_MISMATCH), PROCESS(UNEXPECTED_TOKEN)

typedef enum Status {
    FOREACH_STATUS(GENERATE_ENUM),
} Status;

static const char* const STATUS_TYPE_NAMES[] = {
    FOREACH_STATUS(GENERATE_STRING),
};

#define STATUS_OK(expr) expr == SUCCESS
#define STATUS_ERR(expr) expr != SUCCESS

const char* status_name(Status status);

// Discards the status code. For use with `IGNORE_STATUS`
void status_ignore(Status status);

#if defined(__GNUC__) || defined(__clang__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

// Custom function return type.
//
// Forces function to return a status code, which the caller must contend with.
#define TRY_STATUS WARN_UNUSED_RESULT Status

#define IGNORE_STATUS(expr)                   \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        status_ignore(_stat_obfuscated);      \
    } while (0)
