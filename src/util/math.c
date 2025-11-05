#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "util/math.h"

uint32_t ceil_power_of_two_32(uint32_t n) {
    if (n == 0) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

uint64_t ceil_power_of_two_64(uint64_t n) {
    if (n == 0) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;

    return n;
}

bool is_power_of_two(size_t n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

bool approx_eq_float(float x, float y, float tolerance) {
    return approx_eq_double(x, y, tolerance);
}

bool approx_eq_double(double x, double y, double tolerance) {
    assert(tolerance >= 0.0);

    if (x == y) {
        return true;
    } else if (isnan(x) || isnan(y)) {
        return false;
    } else {
        return fabs(x - y) <= tolerance;
    }
}
