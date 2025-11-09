#include <assert.h>
#include <stdint.h>

#include "util/mem.h"

Slice slice_from_z(const char* start) {
    return slice_from_s(start, strlen(start));
}

Slice slice_from_s(const char* start, size_t size) {
    return (Slice){.ptr = start, .length = size};
}

bool slice_equals(const Slice* a, const Slice* b) {
    assert(a && b);
    return a->length == b->length && (memcmp(a->ptr, b->ptr, a->length) == 0);
}

bool slice_equals_str_z(const Slice* slice, const char* str) {
    assert(slice);
    if (!slice->ptr && !str) {
        return true;
    }

    return slice_equals_str_s(slice, str, strlen(str));
}

bool slice_equals_str_s(const Slice* slice, const char* str, size_t size) {
    assert(slice);
    if (!slice->ptr && !str) {
        return true;
    }

    return slice->length == size && (memcmp(slice->ptr, str, slice->length) == 0);
}

MutSlice mut_slice_from_z(char* start) {
    return mut_slice_from_s(start, strlen(start));
}

MutSlice mut_slice_from_s(char* start, size_t size) {
    return (MutSlice){.ptr = start, .length = size};
}

bool mut_slice_equals(const MutSlice* a, const MutSlice* b) {
    assert(a && b);
    return a->length == b->length && (memcmp(a->ptr, b->ptr, a->length) == 0);
}

bool mut_slice_equals_str_z(const MutSlice* slice, const char* str) {
    assert(slice);
    if (!slice->ptr && !str) {
        return true;
    }

    return mut_slice_equals_str_s(slice, str, strlen(str));
}

bool mut_slice_equals_str_s(const MutSlice* slice, const char* str, size_t size) {
    assert(slice);
    if (!slice->ptr && !str) {
        return true;
    }

    return slice->length == size && (memcmp(slice->ptr, str, slice->length) == 0);
}

uintptr_t align_up(uintptr_t ptr, size_t alignment) {
    assert(is_power_of_two(alignment));
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

void* align_ptr(void* ptr, size_t alignment) {
    return (void*)align_up((uintptr_t)ptr, alignment);
}

void* ptr_offset(void* p, size_t offset) {
    return (void*)((uintptr_t)p + offset);
}

void swap(void* a, void* b, size_t size) {
    if (!a || !b) {
        return;
    }

    uint8_t* pa = (uint8_t*)a;
    uint8_t* pb = (uint8_t*)b;

    for (size_t i = 0; i < size; ++i) {
        uint8_t temp = pa[i];
        pa[i]        = pb[i];
        pb[i]        = temp;
    }
}

char* strdup_z(const char* str) {
    if (!str) {
        return NULL;
    }
    return strdup_s(str, strlen(str));
}

char* strdup_s(const char* str, size_t size) {
    if (!str) {
        return NULL;
    }

    char* copy = malloc(size + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, size);
    copy[size] = '\0';
    return copy;
}
