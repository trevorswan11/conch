#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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
