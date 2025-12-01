#pragma once

#include <assert.h>
#include <stdbool.h>

#define ENUMERATE(ENUM) ENUM
#define STRINGIFY(STRING) #STRING

#define TRY_PROPAGATE_ERROR(E) \
    if (E != SUCCESS) {        \
        return E;              \
    }

#define PROPAGATE_IF_ERROR(expr)                  \
    do {                                          \
        const Status _stat_obfuscated_pie = expr; \
        if (_stat_obfuscated_pie != SUCCESS) {    \
            return _stat_obfuscated_pie;          \
        }                                         \
    } while (0)

#define PROPAGATE_IF_ERROR_DO(expr, action)        \
    do {                                           \
        const Status _stat_obfuscated_pied = expr; \
        if (_stat_obfuscated_pied != SUCCESS) {    \
            action;                                \
            return _stat_obfuscated_pied;          \
        }                                          \
    } while (0)

#define PROPAGATE_IF_ERROR_IS(expr, S)             \
    do {                                           \
        const Status _stat_obfuscated_piei = expr; \
        if (_stat_obfuscated_piei == S) {          \
            return _stat_obfuscated_piei;          \
        }                                          \
    } while (0)

#define PROPAGATE_IF_ERROR_DO_IS(expr, action, S)   \
    do {                                            \
        const Status _stat_obfuscated_piedi = expr; \
        if (_stat_obfuscated_piedi == S) {          \
            action;                                 \
            return _stat_obfuscated_piedi;          \
        }                                           \
    } while (0)

#define PROPAGATE_IF_ERROR_NOT(expr, S)            \
    do {                                           \
        const Status _stat_obfuscated_pien = expr; \
        if (_stat_obfuscated_pien != S) {          \
            return _stat_obfuscated_pien;          \
        }                                          \
    } while (0)

// Propagates the `GENERAL_IO` status code if the io expression evaluates to a negative int.
#define PROPAGATE_IF_IO_ERROR(io_expr) \
    do {                               \
        if (io_expr < 0) {             \
            return GENERAL_IO_ERROR;   \
        }                              \
    } while (0)

#define PROPAGATE_IF_IO_ERROR_DO(io_expr, action) \
    do {                                          \
        if (io_expr < 0) {                        \
            action;                               \
            return GENERAL_IO_ERROR;              \
        }                                         \
    } while (0)

// Allows a variable to be left unused.
//
// For unused error codes, see `UNREACHABLE_IF_ERROR` or `IGNORE_STATUS`.
#define MAYBE_UNUSED(x) ((void)(x))

// Prints to stderr without considering the chance of IO failure.
void debug_print(const char* format, ...);

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE_IMPL \
    assert(0);           \
    __builtin_unreachable();
#elif defined(_MSC_VER)
#define UNREACHABLE_IMPL \
    assert(0);           \
    __assume(0);
#else
#define UNREACHABLE_IMPL abort()
#endif

// This raises an assertion when possible, only if the passed expression is not Status::SUCCESS.
//
// A debug message is also emitted with the file and approximate line number.
#define UNREACHABLE_IF_ERROR(expr)                                                        \
    do {                                                                                  \
        const Status _stat_obfuscated_uie = expr;                                         \
        if (_stat_obfuscated_uie != SUCCESS) {                                            \
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

#define UNREACHABLE UNREACHABLE_IF(true)

#define FOREACH_STATUS(PROCESS)                                                                    \
    PROCESS(SUCCESS), PROCESS(ALLOCATION_FAILED), PROCESS(REALLOCATION_FAILED),                    \
        PROCESS(NULL_PARAMETER), PROCESS(VIOLATED_INVARIANT), PROCESS(INDEX_OUT_OF_BOUNDS),        \
        PROCESS(ELEMENT_MISSING), PROCESS(ZERO_ITEM_SIZE), PROCESS(ZERO_ITEM_ALIGN),               \
        PROCESS(EMPTY), PROCESS(SIZE_OVERFLOW), PROCESS(SIGNED_INTEGER_OVERFLOW),                  \
        PROCESS(UNSIGNED_INTEGER_OVERFLOW), PROCESS(FLOAT_OVERFLOW), PROCESS(READ_ERROR),          \
        PROCESS(WRITE_ERROR), PROCESS(GENERAL_IO_ERROR), PROCESS(TYPE_MISMATCH),                   \
        PROCESS(UNEXPECTED_TOKEN), PROCESS(MALFORMED_INTEGER_STR), PROCESS(MALFORMED_FLOAT_STR),   \
        PROCESS(NOT_IMPLEMENTED), PROCESS(DECL_MISSING_TYPE), PROCESS(CONST_DECL_MISSING_VALUE),   \
        PROCESS(FORWARD_VAR_DECL_MISSING_TYPE), PROCESS(MALFORMED_FUNCTION_LITERAL),               \
        PROCESS(ENUM_MISSING_VARIANTS), PROCESS(BUFFER_OVERFLOW), PROCESS(STRUCT_MISSING_MEMBERS), \
        PROCESS(STRUCT_MEMBER_NOT_EXPLICIT), PROCESS(MISSING_TRAILING_COMMA)

typedef enum Status {
    FOREACH_STATUS(ENUMERATE),
} Status;

static const char* const STATUS_TYPE_NAMES[] = {
    FOREACH_STATUS(STRINGIFY),
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

#define IGNORE_STATUS(expr)                      \
    do {                                         \
        const Status _stat_obfuscated_is = expr; \
        status_ignore(_stat_obfuscated_is);      \
    } while (0)
