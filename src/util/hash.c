#include "util/hash.h"
#include "util/memory.h"

int compare_string_z(const void* a, const void* b) {
    const char* str_a = (const char*)a;
    const char* str_b = (const char*)b;
    return strcmp(str_a, str_b);
}

int compare_slices(const void* a, const void* b) {
    const Slice* sa = (const Slice*)a;
    const Slice* sb = (const Slice*)b;

    size_t len = sa->length < sb->length ? sa->length : sb->length;
    int    cmp = memcmp(sa->ptr, sb->ptr, len);
    if (cmp != 0) { return cmp; }

    return (int)(sa->length - sb->length);
}

int compare_mut_slices(const void* a, const void* b) {
    const MutSlice* sa = (const MutSlice*)a;
    const MutSlice* sb = (const MutSlice*)b;

    size_t len = sa->length < sb->length ? sa->length : sb->length;
    int    cmp = memcmp(sa->ptr, sb->ptr, len);
    if (cmp != 0) { return cmp; }

    return (int)(sa->length - sb->length);
}

static inline Hash hash_string_s(const char* str, size_t size) {
    Hash hash = 5381;
    for (size_t i = 0; i < size; i++) {
        char c = str[i];
        hash   = ((hash << 5) + hash) + c;
    }
    return hash;
}

Hash hash_string_z(const void* key) {
    const char* str = (const char*)key;
    return hash_string_s(str, strlen(str));
}

Hash hash_slice(const void* key) {
    const Slice* slice = (const Slice*)key;
    return hash_string_s(slice->ptr, slice->length);
}

Hash hash_mut_slice(const void* key) {
    const MutSlice* slice = (const MutSlice*)key;
    return hash_string_s(slice->ptr, slice->length);
}
