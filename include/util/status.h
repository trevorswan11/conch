#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

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

// Propagates the `GENERAL_IO` status code if the io expression evaluates to a negative int.
#define PROPAGATE_IF_IO_ERROR(io_expr) \
    do {                               \
        if (io_expr < 0) {             \
            return GENERAL_IO_ERROR;   \
        }                              \
    } while (0)

// Allows a variable to be left unused.
//
// For unused error codes, see `UNREACHABLE_ERROR` or `IGNORE_STATUS`.
#define MAYBE_UNUSED(x) ((void)(x))

// Prints to stderr without considering the chance of IO failure.
void debug_print(const char* format, ...);

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE_IMPL         \
    do {                         \
        assert(0);               \
        __builtin_unreachable(); \
    } while (0)
#elif defined(_MSC_VER)
#define UNREACHABLE_IMPL \
    do {                 \
        assert(0);       \
        __assume(0);     \
    } while (0)
#else
#define UNREACHABLE_IMPL abort()
#endif

// This raises an assertion when possible, only if the passed expression is not Status::SUCCESS.
//
// A debug message is also emitted with the file and approximate line number.
#define UNREACHABLE_IF_ERROR(expr)                                                        \
    do {                                                                                  \
        const Status _stat_obfuscated = expr;                                             \
        if (_stat_obfuscated != SUCCESS) {                                                \
            debug_print("Panic: reached unreachable code (%s:%d)\n", __FILE__, __LINE__); \
            UNREACHABLE_IMPL;                                                             \
        }                                                                                 \
    } while (0)

// This raises an assertion when possible, only if the passed expression is true.
//
// A debug message is also emitted with the file and approximate line number.
#define UNREACHABLE_IF(expr)                                                              \
    do {                                                                                  \
        if (expr) {                                                                       \
            debug_print("Panic: reached unreachable code (%s:%d)\n", __FILE__, __LINE__); \
            UNREACHABLE_IMPL;                                                             \
        }                                                                                 \
    } while (0)

#define FOREACH_STATUS(PROCESS)                                                                  \
    PROCESS(SUCCESS), PROCESS(ALLOCATION_FAILED), PROCESS(REALLOCATION_FAILED),                  \
        PROCESS(NULL_PARAMETER), PROCESS(VIOLATED_INVARIANT), PROCESS(INDEX_OUT_OF_BOUNDS),      \
        PROCESS(ELEMENT_MISSING), PROCESS(ZERO_ITEM_SIZE), PROCESS(ZERO_ITEM_ALIGN),             \
        PROCESS(EMPTY), PROCESS(SIZE_OVERFLOW), PROCESS(SIGNED_INTEGER_OVERFLOW),                \
        PROCESS(UNSIGNED_INTEGER_OVERFLOW), PROCESS(FLOAT_OVERFLOW), PROCESS(READ_ERROR),        \
        PROCESS(WRITE_ERROR), PROCESS(GENERAL_IO_ERROR), PROCESS(TYPE_MISMATCH),                 \
        PROCESS(UNEXPECTED_TOKEN), PROCESS(MALFORMED_INTEGER_STR), PROCESS(MALFORMED_FLOAT_STR), \
        PROCESS(NOT_IMPLEMENTED), PROCESS(DECL_MISSING_TYPE), PROCESS(CONST_DECL_MISSING_VALUE), \
        PROCESS(FORWARD_VAR_DECL_MISSING_TYPE), PROCESS(ILLEGAL_DEFAULT_FUNCTION_PARAMETER)

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
#elif defined(_MSC_VER)
#define WARN_UNUSED_RESULT _Check_return_
#else
#define WARN_UNUSED_RESULT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ALLOW_UNUSED_FN __attribute__((unused))
#else
#define ALLOW_UNUSED_FN
#endif

// Custom function return type.
//
// Forces function to return a status code, which the caller must contend with.
// - On GCC/Clang, this is a compile error with '-Wall -Werror'
// - On MSVC, this is only detected through static analysis
#define TRY_STATUS WARN_UNUSED_RESULT Status

#define IGNORE_STATUS(expr)                   \
    do {                                      \
        const Status _stat_obfuscated = expr; \
        status_ignore(_stat_obfuscated);      \
    } while (0)
