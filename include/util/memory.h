#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/status.h"

#if INTPTR_MAX == INT64_MAX
#define WORD_SIZE_64
#elif INTPTR_MAX == INT32_MAX
#define WORD_SIZE_32
#else
#error "Unsupported architecture"
#endif

#define ASSERT_ALLOCATOR(a)     \
    assert(a.memory_alloc);     \
    assert(a.continuous_alloc); \
    assert(a.re_alloc);         \
    assert(a.free_alloc);

typedef void* (*memory_alloc_fn)(size_t);
typedef void* (*continuous_alloc_fn)(size_t, size_t);
typedef void* (*re_alloc_fn)(void*, size_t);
typedef void (*free_alloc_fn)(void*);

typedef struct Allocator {
    memory_alloc_fn     memory_alloc;
    continuous_alloc_fn continuous_alloc;
    re_alloc_fn         re_alloc;
    free_alloc_fn       free_alloc;
} Allocator;

static const Allocator standard_allocator = {
    .memory_alloc     = &malloc,
    .continuous_alloc = &calloc,
    .re_alloc         = &realloc,
    .free_alloc       = &free,
};

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

// Creates a slice from a mutable one.
//
// Makes no assumptions about ownership.
Slice slice_from_mut(const MutSlice* slice);

Slice       zeroed_slice(void);
MutSlice    zeroed_mut_slice(void);
AnySlice    zeroed_any_slice(void);
AnyMutSlice zeroed_any_mut_slice(void);

AnySlice any_from_slice(const Slice* slice);
AnySlice any_from_mut_slice(const MutSlice* slice);
AnySlice any_from_any_mut_slice(const AnyMutSlice* slice);

uintptr_t align_up(uintptr_t ptr, size_t alignment);
void*     align_ptr(void* ptr, size_t alignment);
void*     ptr_offset(void* p, size_t offset);
void      swap(void* a, void* b, size_t size);

NODISCARD char* strdup_z_allocator(const char* str, memory_alloc_fn memory_alloc);
NODISCARD char* strdup_z(const char* str);
NODISCARD char* strdup_s_allocator(const char* str, size_t size, memory_alloc_fn memory_alloc);
NODISCARD char* strdup_s(const char* str, size_t size);

NODISCARD Status slice_dupe(MutSlice* dest, const Slice* src, memory_alloc_fn memory_alloc);
NODISCARD Status mut_slice_dupe(MutSlice* dest, const MutSlice* src, memory_alloc_fn memory_alloc);

typedef void (*rc_dtor)(void*, free_alloc_fn);

// The control block for reference counted objects.
// All operations can only fail if you don't respect memory safety.
//
// This must be the first member in a reference counted struct.
typedef struct {
    size_t  ref_count;
    rc_dtor dtor;
} RcControlBlock;

// Creates the control block for an object.
//
// - The destructor can be null if the object is 'trivial'.
// - If a destructor is provided, it must not free its parent, only its members!
RcControlBlock rc_init(rc_dtor dtor);

// Increments the objects reference count and returns a pointer to itself.
//
// The object must have the control block as its first member.
NODISCARD void* rc_retain(void* rc_obj);

// Reduces the reference count, and frees if this was the last owner of memory.
// Use the RC_RELEASE macro instead of this function!
//
// The object must have the control block as its first member.
void rc_release(void* rc_obj, free_alloc_fn free_alloc);

#ifdef DIST
#define RC_RELEASE(rc_obj, free_alloc) rc_release(rc_obj, free_alloc)
#else
#define RC_RELEASE(rc_obj, free_alloc)          \
    do {                                        \
        if ((rc_obj) != NULL) {                 \
            rc_release((rc_obj), (free_alloc)); \
            (rc_obj) = NULL;                    \
        }                                       \
    } while (0)
#endif
