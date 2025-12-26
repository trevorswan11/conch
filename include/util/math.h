#ifndef MATH_H
#define MATH_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Creates a `max` function for the type `T` which must implicitly be castable from `VA_ARG_TYPE`.
// - For integer types smaller than int (char, short), VA_ARG_TYPE should be int
// - Use double precision floats only
// - For int, double, or size_t, you should have no trouble
//
// This macro assumes all arguments passed are implicitly convertible from `VA_ARG_TYPE`.
#define MAX_FN(T, VA_ARG_TYPE)                       \
    static inline T max_##T(size_t count, ...) {     \
        va_list args;                                \
        va_start(args, count);                       \
        T max_val = va_arg(args, VA_ARG_TYPE);       \
        for (size_t i = 1; i < count; i++) {         \
            const T val = va_arg(args, VA_ARG_TYPE); \
            if (val > max_val) { max_val = val; }    \
        }                                            \
        va_end(args);                                \
        return max_val;                              \
    }

// Rounds up to the next power of two after the provided 32 bit integer.
//
// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
uint32_t ceil_power_of_two_32(uint32_t n);

// Rounds up to the next power of two after the provided 64 bit integer.
//
// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
uint64_t ceil_power_of_two_64(uint64_t n);

size_t ceil_power_of_two_size(size_t size);

// Checks if the provided size is a power of two.
//
// A number is a power of two if it has only one bit set.
bool is_power_of_two(size_t n);

#define EPSILON 1e-6

bool approx_eq_float(float x, float y, float tolerance);
bool approx_eq_double(double x, double y, double tolerance);

#endif
