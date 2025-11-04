#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// A stack allocated slice into a string with a given length.
typedef struct {
    const char* ptr;
    size_t      length;
} Slice;

static inline Slice slice_from(const char* start, size_t length) {
    return (Slice){.ptr = start, .length = length};
}

static inline bool slice_equals(const Slice* a, const Slice* b) {
    return a->length == b->length && memcmp(a->ptr, b->ptr, a->length) == 0;
}

static inline bool slice_equals_str(const Slice* slice, const char* str) {
    size_t str_len = strlen(str);
    return slice->length == str_len && memcmp(slice->ptr, str, slice->length) == 0;
}

static inline uintptr_t align_up(uintptr_t ptr, size_t alignment) {
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

static inline void* align_ptr(void* ptr, size_t alignment) {
    return (void*)align_up((uintptr_t)ptr, alignment);
}

static inline void* ptr_add(void* p, size_t offset) {
    return (void*)((uintptr_t)p + offset);
}
