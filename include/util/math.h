#pragma once

#include <stdarg.h>
#include <stdint.h>

// Creates a `max` function for the type `T` which must implicitly be castable from `VA_ARG_TYPE`.
// - For integer types smaller than int (char, short), VA_ARG_TYPE should be int
// - Use double precision floats only
// - For int, double, or size_t, you should have no trouble
//
// This macro assumes all arguments passed are implicitly convertible from `VA_ARG_TYPE`.
#define MAX_FN(T, VA_ARG_TYPE)                   \
    static inline T max_##T(size_t count, ...) { \
        va_list args;                            \
        va_start(args, count);                   \
        T max_val = va_arg(args, VA_ARG_TYPE);   \
        for (size_t i = 1; i < count; i++) {     \
            T val = va_arg(args, VA_ARG_TYPE);   \
            if (val > max_val) {                 \
                max_val = val;                   \
            }                                    \
        }                                        \
        va_end(args);                            \
        return max_val;                          \
    }
