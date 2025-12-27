#include "util/memory.h"
#include "util/status.h"

#ifndef NDEBUG
#include "util/math.h"
#endif

Slice slice_from_str_z(const char* start) { return slice_from_str_s(start, strlen(start)); }

Slice slice_from_str_s(const char* start, size_t size) {
    return (Slice){.ptr = start, .length = size};
}

bool slice_equals(const Slice* a, const Slice* b) {
    assert(a && b);
    return a->length == b->length && (memcmp(a->ptr, b->ptr, a->length) == 0);
}

bool slice_equals_str_z(const Slice* slice, const char* str) {
    assert(slice);
    if (!slice->ptr && !str) { return true; }

    return slice_equals_str_s(slice, str, strlen(str));
}

bool slice_equals_str_s(const Slice* slice, const char* str, size_t size) {
    assert(slice);
    if (!slice->ptr && !str) { return true; }

    return slice->length == size && (memcmp(slice->ptr, str, slice->length) == 0);
}

MutSlice mut_slice_from_str_z(char* start) { return mut_slice_from_str_s(start, strlen(start)); }

MutSlice mut_slice_from_str_s(char* start, size_t size) {
    return (MutSlice){.ptr = start, .length = size};
}

bool mut_slice_equals(const MutSlice* a, const MutSlice* b) {
    assert(a && b);
    return a->length == b->length && (memcmp(a->ptr, b->ptr, a->length) == 0);
}

bool mut_slice_equals_str_z(const MutSlice* slice, const char* str) {
    assert(slice);
    if (!slice->ptr && !str) { return true; }

    return mut_slice_equals_str_s(slice, str, strlen(str));
}

bool mut_slice_equals_str_s(const MutSlice* slice, const char* str, size_t size) {
    assert(slice);
    if (!slice->ptr && !str) { return true; }

    return slice->length == size && (memcmp(slice->ptr, str, slice->length) == 0);
}

Slice slice_from_mut(const MutSlice* slice) { return slice_from_str_s(slice->ptr, slice->length); }

Slice       zeroed_slice(void) { return slice_from_str_s(nullptr, 0); }
MutSlice    zeroed_mut_slice(void) { return mut_slice_from_str_s(nullptr, 0); }
AnySlice    zeroed_any_slice(void) { return (AnySlice){nullptr, 0}; }
AnyMutSlice zeroed_any_mut_slice(void) { return (AnyMutSlice){nullptr, 0}; }

AnySlice any_from_slice(const Slice* slice) { return (AnySlice){slice->ptr, slice->length}; }
AnySlice any_from_mut_slice(const MutSlice* slice) { return (AnySlice){slice->ptr, slice->length}; }
AnySlice any_from_any_mut_slice(const AnyMutSlice* slice) {
    return (AnySlice){slice->ptr, slice->length};
}

uintptr_t align_up(uintptr_t ptr, size_t alignment) {
    assert(is_power_of_two(alignment));
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

void* align_ptr(void* ptr, size_t alignment) { return (void*)align_up((uintptr_t)ptr, alignment); }
void* ptr_offset(void* p, size_t offset) { return (void*)((uintptr_t)p + offset); }

void swap(void* a, void* b, size_t size) {
    if (!a || !b) { return; }

    uint8_t* pa = (uint8_t*)a;
    uint8_t* pb = (uint8_t*)b;

    for (size_t i = 0; i < size; ++i) {
        uint8_t temp = pa[i];
        pa[i]        = pb[i];
        pb[i]        = temp;
    }
}

[[nodiscard]] char* strdup_z_allocator(const char* str, Allocator* allocator) {
    if (!str) { return nullptr; }
    return strdup_s_allocator(str, strlen(str), allocator);
}

[[nodiscard]] char* strdup_z(const char* str) { return strdup_z_allocator(str, &std_allocator); }

[[nodiscard]] char* strdup_s_allocator(const char* str, size_t size, Allocator* allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    if (!str) { return nullptr; }

    char* copy = ALLOCATOR_PTR_MALLOC(allocator, size + 1);
    if (!copy) { return nullptr; }

    memcpy(copy, str, size);
    copy[size] = '\0';
    return copy;
}

[[nodiscard]] char* strdup_s(const char* str, size_t size) {
    return strdup_s_allocator(str, size, &std_allocator);
}

[[nodiscard]] Status slice_dupe(MutSlice* dest, const Slice* src, Allocator* allocator) {
    assert(src && dest);
    char* duped = strdup_s_allocator(src->ptr, src->length, allocator);
    if (!duped) { return ALLOCATION_FAILED; }

    *dest = mut_slice_from_str_s(duped, src->length);
    return SUCCESS;
}

[[nodiscard]] Status mut_slice_dupe(MutSlice* dest, const MutSlice* src, Allocator* allocator) {
    const Slice slice_src = slice_from_mut(src);
    return slice_dupe(dest, &slice_src, allocator);
}

RcControlBlock rc_init(rc_dtor dtor) {
    return (RcControlBlock){
        .ref_count = 1,
        .dtor      = dtor,
    };
}

[[nodiscard]] void* rc_retain(void* rc_obj) {
    assert(rc_obj);
    RcControlBlock* rc = (RcControlBlock*)rc_obj;
    rc->ref_count += 1;
    return rc_obj;
}

void rc_release(void* rc_obj, Allocator* allocator) {
    if (!rc_obj) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    RcControlBlock* rc = (RcControlBlock*)rc_obj;
    rc->ref_count -= 1;

    if (rc->ref_count == 0) {
        if (rc->dtor) { rc->dtor(rc_obj, allocator); }
        ALLOCATOR_PTR_FREE(allocator, rc_obj);
    }
}
