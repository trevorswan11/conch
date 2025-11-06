#pragma once

#include <stdint.h>

// Compares two null terminated strings lexigraphically.
//
// Assumes that the string is null terminated, without checking.
int compare_string_z(const void* a, const void* b);
int compare_slices(const void* a, const void* b);

// Hashes a null terminated string.
//
// Uses the djb2 algorithm by Dan Bernstein.
//
// Assumes that the string is null terminated, without checking.
uint64_t hash_string_z(const void* key);
uint64_t hash_slice(const void* key);

#define HASH_CONCAT_2(a, b) a##b
#define HASH_CONCAT_3(a, b, c) a##b##c

// Generates a generic hash function for the given unsigned integer.
// It's corresponding signed integer type is also generated and uses the unsigned variant
// internally. Use the integer types from stdint.h with this macro.
//
// Hash functions accept 8, 16, 32, and 64 bit integers, others are undefined and return 0.
//
// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
#define HASH_INTEGER_FN(T)                                                                        \
    static inline uint64_t HASH_CONCAT_3(hash_, T, _u)(const void* key) {                         \
        T        t   = *(const T*)key;                                                            \
        uint64_t val = (uint64_t)(T)t;                                                            \
        if (sizeof(T) == 1) {                                                                     \
            val = (val ^ 0xAAU) + (val << 3);                                                     \
            val = val ^ (val >> 4);                                                               \
            val = val * 0x27U;                                                                    \
            return val;                                                                           \
        } else if (sizeof(T) == 2) {                                                              \
            val = (val ^ 0xAAAAU) + (val << 7);                                                   \
            val = val ^ (val >> 9);                                                               \
            val = val * 0x27D4U;                                                                  \
            val = val ^ (val >> 11);                                                              \
            return val;                                                                           \
        } else if (sizeof(T) == 4) {                                                              \
            val = ((val >> 16) ^ val) * 0x45D9F3BU;                                               \
            val = ((val >> 16) ^ val) * 0x45D9F3BU;                                               \
            val = (val >> 16) ^ val;                                                              \
            return val;                                                                           \
        } else if (sizeof(T) == 8) {                                                              \
            val = (val ^ (val >> 30)) * 0xBF58476D1CE4E5B9ULL;                                    \
            val = (val ^ (val >> 27)) * 0x94D049BB133111EBULL;                                    \
            val = val ^ (val >> 31);                                                              \
            return val;                                                                           \
        } else {                                                                                  \
            return 0;                                                                             \
        }                                                                                         \
    }                                                                                             \
                                                                                                  \
    static inline __attribute__((unused)) uint64_t HASH_CONCAT_3(hash_, T, _s)(const void* key) { \
        T ux = *(const T*)key;                                                                    \
        return HASH_CONCAT_3(hash_, T, _u)(&ux);                                                  \
    }

// Generates a compare function for the integer type.
// Use the integer types from stdint.h with this macro.
//
// This function takes two void pointers and subtracts them.
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
#define COMPARE_INTEGER_FN(T)                                                    \
    static inline int HASH_CONCAT_2(compare_, T)(const void* a, const void* b) { \
        const T va = *(const T*)a;                                               \
        const T vb = *(const T*)b;                                               \
        return (va > vb) - (va < vb);                                            \
    }
