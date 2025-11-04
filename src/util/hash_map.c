#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "util/hash_map.h"
#include "util/mem.h"

bool hash_map_init(HashMap* hm,
                   size_t   capacity,
                   size_t   key_size,
                   size_t   key_align,
                   size_t   value_size,
                   size_t   value_align,
                   Hash (*hash)(const void*),
                   int (*compare)(const void*, const void*)) {
    if (!hm || !hash || !compare) {
        return false;
    } else if (key_size == 0 || value_size == 0 || key_align == 0 || value_align == 0) {
        return false;
    }

    capacity = capacity < MINIMUM_CAPACITY ? MINIMUM_CAPACITY : capacity;

    const size_t header_size   = sizeof(Header);
    const size_t metadata_size = sizeof(Metadata) * capacity;
    const size_t keys_size     = key_size * capacity;
    const size_t values_size   = value_size * capacity;
    const size_t entries_size  = sizeof(Entry) * capacity;

    // Safety checks before allocating for overflow guard
    if (capacity > SIZE_MAX / key_size) {
        return false;
    } else if (capacity > SIZE_MAX / value_size) {
        return false;
    } else if (metadata_size > SIZE_MAX - header_size) {
        return false;
    }

    const size_t total_size = header_size + metadata_size + keys_size + values_size + entries_size;
    void*        buffer     = calloc(1, total_size);
    if (!buffer) {
        return false;
    }

    // Unpack the allocated block of memory
    uintptr_t ptr    = (uintptr_t)buffer;
    Header*   header = (Header*)ptr;
    ptr += sizeof(Header);

    Metadata* metadata = (Metadata*)ptr;
    ptr += sizeof(Metadata) * capacity;

    void* keys = align_ptr((void*)ptr, key_align);
    ptr        = (uintptr_t)keys + key_size * capacity;

    void* values = align_ptr((void*)ptr, value_align);
    ptr          = (uintptr_t)values + value_size * capacity;

    Entry* entries = align_ptr((void*)ptr, alignof(Entry));

    assert(((uintptr_t)keys % key_align) == 0);
    assert(((uintptr_t)values % value_align) == 0);
    assert(((uintptr_t)entries % alignof(Entry)) == 0);

    // Fill header info
    *header = (Header){
        .keys        = keys,
        .key_size    = key_size,
        .key_align   = key_align,
        .values      = values,
        .value_size  = value_size,
        .value_align = value_align,
        .capacity    = capacity,
    };

    // Fill remaining pointers
    *hm = (HashMap){
        .buffer    = buffer,
        .header    = header,
        .metadata  = metadata,
        .entries   = entries,
        .size      = 0,
        .available = capacity,
        .hash      = hash,
        .compare   = compare,
    };

    // Initialize metadata to open
    for (size_t i = 0; i < capacity; i++) {
        hm->metadata[i] = METADATA_SLOT_OPEN;
    }

    return true;
}

void hash_map_deinit(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return;
    }

    free(hm->buffer);
    hm->buffer    = NULL;
    hm->header    = NULL;
    hm->metadata  = NULL;
    hm->entries   = NULL;
    hm->size      = 0;
    hm->available = 0;
}
