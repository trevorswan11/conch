#include "util/hash.h"
#include "util/mem.h"

int compare_string_z(const void* a, const void* b) {
    Slice slice_a = slice_from_z((const char*)a);
    Slice slice_b = slice_from_z((const char*)b);
    return compare_slices(&slice_a, &slice_b);
}

int compare_slices(const void* a, const void* b) {
    const Slice* slice_a = (const Slice*)a;
    const Slice* slice_b = (const Slice*)b;
    return slice_equals(slice_a, slice_b);
}

uint64_t hash_string_z(const void* key) {
    Slice slice = slice_from_z((const char*)key);
    return hash_slice(&slice);
}

uint64_t hash_slice(const void* key) {
    const Slice* slice = (const Slice*)key;
    uint64_t     hash  = 5381;

    for (size_t i = 0; i < slice->length; i++) {
        char c = slice->ptr[i];
        hash   = ((hash << 5) + hash) + c;
    }

    return hash;
}
