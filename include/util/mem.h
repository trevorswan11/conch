#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "util/math.h"

#if INTPTR_MAX == INT64_MAX
#define WORD_SIZE_64
#elif INTPTR_MAX == INT32_MAX
#define WORD_SIZE_32
#else
#error "Unsupported architecture"
#endif

// A stack allocated slice into a string with a given length.
typedef struct {
    const char* ptr;
    size_t      length;
} Slice;

// A stack allocated slice into arbitrary memory with a given length.
typedef struct {
    const void* ptr;
    size_t      length;
} AnySlice;

// Creates a slice from a null terminated string.
Slice slice_from_str_z(const char* start);
Slice slice_from_str_s(const char* start, size_t size);
bool  slice_equals(const Slice* a, const Slice* b);

// Compares a slice to a null terminated string.
bool slice_equals_str_z(const Slice* slice, const char* str);
bool slice_equals_str_s(const Slice* slice, const char* str, size_t size);

// A stack allocated slice into a mutable string with a given length.
typedef struct {
    char*  ptr;
    size_t length;
} MutSlice;

// A stack allocated slice into mutable arbitrary memory with a given length.
typedef struct {
    void*  ptr;
    size_t length;
} AnyMutSlice;

// Creates a mutable slice from a null terminated string.
MutSlice mut_slice_from_str_z(char* start);
MutSlice mut_slice_from_str_s(char* start, size_t size);
bool     mut_slice_equals(const MutSlice* a, const MutSlice* b);

// Compares a mutable slice to a null terminated string.
bool mut_slice_equals_str_z(const MutSlice* slice, const char* str);
bool mut_slice_equals_str_s(const MutSlice* slice, const char* str, size_t size);

static inline bool mut_slice_equals_slice(const MutSlice* a, const Slice* b) {
    return mut_slice_equals_str_s(a, b->ptr, b->length);
}

uintptr_t align_up(uintptr_t ptr, size_t alignment);
void*     align_ptr(void* ptr, size_t alignment);
void*     ptr_offset(void* p, size_t offset);
void      swap(void* a, void* b, size_t size);

char* strdup_z(const char* str);
char* strdup_s(const char* str, size_t size);
