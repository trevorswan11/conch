#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define ENUMERATE(ENUM) ENUM
#define STRINGIFY(STRING) #STRING

#define TRY(expr)                                 \
    do {                                          \
        const Status _stat_obfuscated_pie = expr; \
        if (_stat_obfuscated_pie != SUCCESS) {    \
            return _stat_obfuscated_pie;          \
        }                                         \
    } while (0)

#define TRY_DO(expr, action)                       \
    do {                                           \
        const Status _stat_obfuscated_pied = expr; \
        if (_stat_obfuscated_pied != SUCCESS) {    \
            action;                                \
            return _stat_obfuscated_pied;          \
        }                                          \
    } while (0)

#define TRY_IS(expr, S)                            \
    do {                                           \
        const Status _stat_obfuscated_piei = expr; \
        if (_stat_obfuscated_piei == S) {          \
            return _stat_obfuscated_piei;          \
        }                                          \
    } while (0)

#define TRY_DO_IS(expr, action, S)                  \
    do {                                            \
        const Status _stat_obfuscated_piedi = expr; \
        if (_stat_obfuscated_piedi == S) {          \
            action;                                 \
            return _stat_obfuscated_piedi;          \
        }                                           \
    } while (0)

#define TRY_NOT(expr, S)                           \
    do {                                           \
        const Status _stat_obfuscated_pien = expr; \
        if (_stat_obfuscated_pien != S) {          \
            return _stat_obfuscated_pien;          \
        }                                          \
    } while (0)

// Propagates the `GENERAL_IO` status code if the io expression evaluates to a negative int.
#define TRY_IO(io_expr)              \
    do {                             \
        if (io_expr < 0) {           \
            return GENERAL_IO_ERROR; \
        }                            \
    } while (0)

#define TRY_IO_DO(io_expr, action)   \
    do {                             \
        if (io_expr < 0) {           \
            action;                  \
            return GENERAL_IO_ERROR; \
        }                            \
    } while (0)

// Allows a variable to be left unused.
//
// For unused error codes, see `UNREACHABLE_IF_ERROR` or `IGNORE_STATUS`.
#define MAYBE_UNUSED(x) ((void)(x))

// Prints to stderr without considering the chance of IO failure.
void debug_print(const char* format, ...);

// This raises an assertion when possible, only if the passed expression is true.
//
// A debug message is also emitted with the file and approximate line number.
#ifdef DIST
#define UNREACHABLE_IF(expr) IGNORE_STATUS(expr)
#define UNREACHABLE_IF_ERROR(expr) IGNORE_STATUS(expr)
#define UNREACHABLE
#else
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

#define UNREACHABLE_IF(expr)                                                              \
    do {                                                                                  \
        if (expr) {                                                                       \
            debug_print("Panic: reached unreachable code (%s:%d)\n", __FILE__, __LINE__); \
            UNREACHABLE_IMPL;                                                             \
        }                                                                                 \
    } while (0)
#define UNREACHABLE_IF_ERROR(expr) UNREACHABLE_IF(STATUS_ERR(expr));
#define UNREACHABLE UNREACHABLE_IF(true)
#endif

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
        PROCESS(STRUCT_MEMBER_NOT_EXPLICIT), PROCESS(MISSING_TRAILING_COMMA),                      \
        PROCESS(EMPTY_IMPL_BLOCK), PROCESS(ARMLESS_MATCH_EXPR), PROCESS(ILLEGAL_MATCH_ARM),        \
        PROCESS(UNEXPECTED_ARRAY_SIZE_TOKEN), PROCESS(MISSING_ARRAY_SIZE_TOKEN),                   \
        PROCESS(INCORRECT_EXPLICIT_ARRAY_SIZE), PROCESS(LOOP_MISSING_BODY),                        \
        PROCESS(FOR_MISSING_ITERABLES), PROCESS(WHILE_MISSING_CONDITION), PROCESS(EMPTY_FOR_LOOP), \
        PROCESS(EMPTY_WHILE_LOOP), PROCESS(ILLEGAL_LOOP_NON_BREAK), PROCESS(ILLEGAL_IF_BRANCH),    \
        PROCESS(FOR_ITERABLE_CAPTURE_MISMATCH), PROCESS(IMPROPER_WHILE_CONTINUATION),              \
        PROCESS(ILLEGAL_IDENTIFIER), PROCESS(EMPTY_GENERIC_LIST), PROCESS(IMPLICIT_FN_PARAM_TYPE), \
        PROCESS(MALFORMED_CHARATCER_LITERAL), PROCESS(USER_IMPORT_MISSING_ALIAS),                  \
        PROCESS(REDEFINITION_OF_IDENTIFIER), PROCESS(UNDECLARED_IDENTIFIER),                       \
        PROCESS(ASSIGNMENT_TO_CONSTANT), PROCESS(MALFORMED_TYPE_DECL), PROCESS(DOUBLE_NULLABLE)

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
#define NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ALLOW_UNUSED_FN __attribute__((unused))
#else
#define ALLOW_UNUSED_FN
#endif

#define IGNORE_STATUS(expr) status_ignore(expr)

typedef struct ArrayList     ArrayList;
typedef struct StringBuilder StringBuilder;

NODISCARD Status put_status_error(ArrayList* errors, Status status, size_t line, size_t col);
NODISCARD Status error_append_ln_col(size_t line, size_t col, StringBuilder* sb);
