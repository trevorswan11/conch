#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/containers/hash_map.h"
#include "util/containers/hash_set.h"
#include "util/math.h"

MAX_FN(size_t, size_t)

// Determines the new capacity based on the current size.
static inline size_t hash_set_capacity_for_size(size_t size) {
    size_t new_cap = ((size * 100) / HASH_SET_MAX_LOAD_PERCENTAGE) + 1;
    return ceil_power_of_two_size(new_cap);
}

// Grows the set to the new capacity, rehashing along the way.
[[nodiscard]] static inline Status hash_set_grow(HashSet* hs, size_t new_capacity) {
    new_capacity = max_size_t(2, new_capacity, HASH_SET_MINIMUM_CAPACITY);
    assert(new_capacity > hs->header->capacity);
    assert(is_power_of_two(new_capacity));

    HashSet set;
    TRY(hash_set_init(
        &set, new_capacity, hs->header->key_size, hs->header->key_align, hs->hash, hs->compare));

    // Append all data to the set without checking sizes
    if (hs->size != 0) {
        const size_t old_capacity = hs->header->capacity;
        for (size_t i = 0; i < old_capacity; i++) {
            if (!metadata_used(hs->metadata[i])) { continue; }

            const void* key = ptr_offset(hs->header->keys, i * hs->header->key_size);
            hash_set_put_assume_capacity_no_clobber(&set, key);

            if (set.size == hs->size) { break; }
        }
    }
    set.available = ((new_capacity * HASH_SET_MAX_LOAD_PERCENTAGE) / 100) - set.size;

    hash_set_deinit(hs);
    *hs = set;
    return SUCCESS;
}

// Only grows if the requested count exceeds the current number of available slots.
[[nodiscard]] static inline Status hash_set_grow_if_needed(HashSet* hs, size_t new_count) {
    const size_t max_load = (hs->header->capacity * HASH_SET_MAX_LOAD_PERCENTAGE) / 100;
    if (hs->size + new_count <= max_load) { return SUCCESS; }

    const size_t desired = hash_set_capacity_for_size(hs->size + new_count);
    TRY(hash_set_grow(hs, desired));
    return SUCCESS;
}

static inline void hash_set_init_metadatas(HashSet* hs) {
    assert(METADATA_SLOT_OPEN.fingerprint == 0);
    assert(METADATA_SLOT_OPEN.used == 0);
    memset(hs->metadata, 0, sizeof(Metadata) * hs->header->capacity);
}

[[nodiscard]] Status hash_set_init_allocator(HashSet* hs,
                                             size_t   capacity,
                                             size_t   key_size,
                                             size_t   key_align,
                                             Hash (*hash)(const void*),
                                             int (*compare)(const void*, const void*),
                                             Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);
    if (!hs || !hash || !compare) { return NULL_PARAMETER; }
    if (key_size == 0 || key_align == 0) { return ZERO_ITEM_SIZE; }

    capacity = capacity < HASH_SET_MINIMUM_CAPACITY ? HASH_SET_MINIMUM_CAPACITY : capacity;
    if (!is_power_of_two(capacity)) { capacity = ceil_power_of_two_size(capacity); }

    const size_t header_size   = sizeof(SetHeader);
    const size_t metadata_size = sizeof(Metadata) * capacity;
    const size_t keys_size     = key_size * capacity;

    // Safety checks before allocating for overflow guard
    if (capacity > SIZE_MAX / key_size) { return SIZE_OVERFLOW; }
    if (metadata_size > SIZE_MAX - header_size) { return SIZE_OVERFLOW; }

    const size_t total_size = header_size + metadata_size + (keys_size + key_align - 1);
    void*        buffer     = allocator.continuous_alloc(1, total_size);
    if (!buffer) { return ALLOCATION_FAILED; }

    // Unpack the allocated block of memory
    uintptr_t ptr = (uintptr_t)buffer;

    SetHeader* header = (SetHeader*)ptr;
    ptr               = (uintptr_t)ptr_offset((void*)ptr, sizeof(SetHeader));

    Metadata* metadata = (Metadata*)ptr;
    ptr                = (uintptr_t)ptr_offset((void*)ptr, sizeof(Metadata) * capacity);

    void* keys = align_ptr((void*)ptr, key_align);
    ptr        = (uintptr_t)ptr_offset(keys, key_size * capacity);

    assert(((uintptr_t)keys % key_align) == 0);

    // Fill header info
    *header = (SetHeader){
        .keys      = keys,
        .key_size  = key_size,
        .key_align = key_align,
        .capacity  = capacity,
    };

    // Fill remaining pointers
    *hs = (HashSet){
        .buffer    = buffer,
        .header    = header,
        .metadata  = metadata,
        .size      = 0,
        .available = capacity,
        .hash      = hash,
        .compare   = compare,
        .allocator = allocator,
    };

    hash_set_init_metadatas(hs);
    return SUCCESS;
}

[[nodiscard]] Status hash_set_init(HashSet* hs,
                                   size_t   capacity,
                                   size_t   key_size,
                                   size_t   key_align,
                                   Hash (*hash)(const void*),
                                   int (*compare)(const void*, const void*)) {
    return hash_set_init_allocator(
        hs, capacity, key_size, key_align, hash, compare, STANDARD_ALLOCATOR);
}

void hash_set_deinit(HashSet* hs) {
    if (!hs || !hs->buffer) { return; }
    ASSERT_ALLOCATOR(hs->allocator);

    hs->allocator.free_alloc(hs->buffer);
    hs->buffer    = nullptr;
    hs->header    = nullptr;
    hs->metadata  = nullptr;
    hs->size      = 0;
    hs->available = 0;
}

size_t hash_set_capacity(const HashSet* hs) {
    assert(hs && hs->buffer);
    return hs->header->capacity;
}

size_t hash_set_count(const HashSet* hs) {
    assert(hs && hs->buffer);
    return hs->size;
}

void hash_set_clear_retaining_capacity(HashSet* hs) {
    hash_set_init_metadatas(hs);
    hs->size      = 0;
    hs->available = (hs->header->capacity * HASH_SET_MAX_LOAD_PERCENTAGE) / 100;
}

[[nodiscard]] Status hash_set_ensure_total_capacity(HashSet* hs, size_t new_size) {
    if (!hs || !hs->buffer) { return NULL_PARAMETER; }

    if (new_size > hs->size) {
        TRY_IS(hash_set_grow_if_needed(hs, new_size - hs->size), ALLOCATION_FAILED);
    }
    return SUCCESS;
}

[[nodiscard]] Status hash_set_ensure_unused_capacity(HashSet* hs, size_t additional_size) {
    return hash_set_ensure_total_capacity(hs, hs->size + additional_size);
}

void hash_set_rehash(HashSet* hs) {
    assert(hs && hs->buffer);

    const size_t capacity = hs->header->capacity;
    const size_t mask     = hs->header->capacity - 1;

    Metadata* metadata = hs->metadata;
    void*     keys_ptr = hs->header->keys;
    size_t    current  = 0;

    // Use fingerprint to mark used buckets as being used and either free
    // (needing to be rehashed) or tombstone (already rehashed).
    while (current < capacity) {
        metadata[current++].fingerprint = FP_OPEN;
    }

    // Now iterate over all the buckets, rehashing them
    current = 0;
    while (current < capacity) {
        if (!metadata_used(metadata[current])) {
            assert(metadata_open(metadata[current]));
            current += 1;
            continue;
        }

        const Hash    hash        = hs->hash(ptr_offset(keys_ptr, current * hs->header->key_size));
        const uint8_t fingerprint = take_fingerprint(hash);
        size_t        probe       = hash & mask;

        // For each bucket, rehash to an index:
        // 1) before the cursor, probed into a free slot, or
        // 2) equal to the cursor, no need to move, or
        // 3) ahead of the cursor, probing over already rehashed
        while ((probe < current && metadata_used(metadata[probe])) ||
               (probe > current && metadata[probe].fingerprint == FP_TOMBSTONE)) {
            probe = (probe + 1) & mask;
        }

        if (probe < current) {
            assert(metadata_open(metadata[probe]));
            metadata_fill(&metadata[probe], fingerprint);

            const void* old_key = ptr_offset(keys_ptr, current * hs->header->key_size);
            void*       new_key = ptr_offset(keys_ptr, probe * hs->header->key_size);

            memcpy(new_key, old_key, hs->header->key_size);

            metadata[current].used = 0;
            assert(metadata_open(metadata[current]));
        } else if (probe == current) {
            metadata_fill(&metadata[probe], fingerprint);
        } else {
            assert(metadata[probe].fingerprint != FP_TOMBSTONE);
            metadata[probe].fingerprint = FP_TOMBSTONE;

            if (metadata_used(metadata[probe])) {
                swap(ptr_offset(keys_ptr, current * hs->header->key_size),
                     ptr_offset(keys_ptr, probe * hs->header->key_size),
                     hs->header->key_size);
                continue;
            }

            metadata[probe].used = 1;
            const void* old_key  = ptr_offset(keys_ptr, current * hs->header->key_size);
            void*       new_key  = ptr_offset(keys_ptr, probe * hs->header->key_size);

            memcpy(new_key, old_key, hs->header->key_size);
            metadata_clear(&metadata[current]);
        }

        current += 1;
    }

    // Convert graveyard into real fingerprints
    for (size_t i = 0; i < capacity; i++) {
        if (metadata[i].fingerprint == FP_TOMBSTONE) {
            // This was a slot > current that was processed
            assert(metadata_used(metadata[i]));
            const Hash hash         = hs->hash(ptr_offset(keys_ptr, i * hs->header->key_size));
            metadata[i].fingerprint = take_fingerprint(hash);
        } else {
            // This was either:
            // 1. A slot <= current that was processed (has correct fp)
            // 2. An empty slot (fp=FP_OPEN, used=0)
            assert(metadata_used(metadata[i]) || metadata_open(metadata[i]));
        }
    }

    hs->available = ((hs->header->capacity * HASH_SET_MAX_LOAD_PERCENTAGE) / 100) - hs->size;
}

void hash_set_put_assume_capacity_no_clobber(HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);
    assert(!hash_set_contains(hs, key));

    const Hash   hash = hs->hash(key);
    const size_t mask = hs->header->capacity - 1;

    size_t probe = hash & mask;

    Metadata* metadata = hs->metadata;

    while (metadata_used(metadata[probe])) {
        probe = (probe + 1) & mask;
    }

    assert(hs->available > 0);
    hs->available -= 1;

    const uint8_t fingerprint = take_fingerprint(hash);
    metadata_fill(&hs->metadata[probe], fingerprint);

    void* key_slot = ptr_offset(hs->header->keys, probe * hs->header->key_size);
    memcpy(key_slot, key, hs->header->key_size);

    hs->size += 1;
}

[[nodiscard]] Status hash_set_put_no_clobber(HashSet* hs, const void* key) {
    TRY_IS(hash_set_grow_if_needed(hs, 1), ALLOCATION_FAILED);

    hash_set_put_assume_capacity_no_clobber(hs, key);
    return true;
}

// The data in the returned result is garbage if an existing key was not found.
SetGetOrPutResult hash_set_get_or_put_assume_capacity(HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);

    const Hash    hash        = hs->hash(key);
    const size_t  mask        = hs->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hs->header->capacity;
    size_t probe = hash & mask;

    size_t    first_tombstone_idx = limit;
    Metadata* metadata            = hs->metadata;

    Metadata m = metadata[probe];
    while (!metadata_open(m) && limit != 0) {
        if (metadata_used(m) && m.fingerprint == fingerprint) {
            void* test_key = ptr_offset(hs->header->keys, probe * hs->header->key_size);

            if (hs->compare(key, test_key) == 0) {
                return (SetGetOrPutResult){
                    .key_ptr        = test_key,
                    .found_existing = true,
                };
            }
        } else if (first_tombstone_idx == hs->header->capacity && metadata_tombstone(m)) {
            first_tombstone_idx = probe;
        }

        limit -= 1;
        probe = (probe + 1) & mask;
        m     = metadata[probe];
    }

    // It's cheaper to lower probing distance after deletions by recycling a tombstone
    if (first_tombstone_idx < hs->header->capacity) { probe = first_tombstone_idx; }
    hs->available -= 1;

    metadata_fill(&hs->metadata[probe], fingerprint);
    hs->size += 1;

    return (SetGetOrPutResult){
        .key_ptr        = ptr_offset(hs->header->keys, probe * hs->header->key_size),
        .found_existing = false,
    };
}

[[nodiscard]] Status hash_set_get_or_put(HashSet* hs, const void* key, SetGetOrPutResult* result) {
    assert(hs && hs->buffer && key);

    // If we fail to grow, still try to find the key
    if (hash_set_grow_if_needed(hs, 1) == ALLOCATION_FAILED) {
        if (hs->available > 0) {
            *result = hash_set_get_or_put_assume_capacity(hs, key);
            return true;
        }

        size_t index;
        TRY(hash_set_get_index(hs, key, &index));

        *result = (SetGetOrPutResult){
            .key_ptr        = ptr_offset(hs->header->keys, index * hs->header->key_size),
            .found_existing = true,
        };
        return SUCCESS;
    }

    *result = hash_set_get_or_put_assume_capacity(hs, key);
    return SUCCESS;
}

void hash_set_put_assume_capacity(HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);
    SetGetOrPutResult gop = hash_set_get_or_put_assume_capacity(hs, key);

    memcpy(gop.key_ptr, key, hs->header->key_size);
}

[[nodiscard]] Status hash_set_put(HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);
    SetGetOrPutResult gop;
    TRY(hash_set_get_or_put(hs, key, &gop));

    memcpy(gop.key_ptr, key, hs->header->key_size);
    return SUCCESS;
}

bool hash_set_contains(const HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);
    return STATUS_OK(hash_set_get_index(hs, key, nullptr));
}

[[nodiscard]] Status hash_set_get_index(const HashSet* hs, const void* key, size_t* index) {
    assert(hs && hs->buffer && key);
    if (hs->size == 0) { return EMPTY; }

    const Hash    hash        = hs->hash(key);
    const size_t  mask        = hs->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hs->header->capacity;
    size_t probe = hash & mask;

    Metadata* metadata = hs->metadata;

    Metadata m = metadata[probe];
    while (!metadata_open(m) && limit != 0) {
        if (metadata_used(m) && m.fingerprint == fingerprint) {
            const void* test_key = ptr_offset(hs->header->keys, probe * hs->header->key_size);

            if (hs->compare(key, test_key) == 0) {
                if (index) { *index = probe; }
                return SUCCESS;
            }
        }

        limit -= 1;
        probe = (probe + 1) & mask;
        m     = metadata[probe];
    }

    return ELEMENT_MISSING;
}

[[nodiscard]] Status hash_set_get_entry(HashSet* hs, const void* key, SetEntry* e) {
    assert(hs && hs->buffer && key);
    size_t index;
    TRY(hash_set_get_index(hs, key, &index));

    *e = (SetEntry){
        .key_ptr = ptr_offset(hs->header->keys, index * hs->header->key_size),
    };
    return SUCCESS;
}

[[nodiscard]] Status hash_set_remove(HashSet* hs, const void* key) {
    assert(hs && hs->buffer && key);
    size_t index;
    TRY(hash_set_get_index(hs, key, &index));

    metadata_remove(&hs->metadata[index]);
    hs->size -= 1;
    hs->available += 1;
    return SUCCESS;
}

HashSetIterator hash_set_iterator_init(HashSet* hs) {
    assert(hs && hs->buffer);
    return (HashSetIterator){
        .hs    = hs,
        .index = 0,
    };
}

bool hash_set_iterator_has_next(HashSetIterator* it, SetEntry* next) {
    assert(it && it->hs && it->hs->buffer);
    assert(it->index <= it->hs->header->capacity);
    if (it->hs->size == 0) { return false; }

    const size_t capacity = it->hs->header->capacity;
    while (it->index < capacity) {
        const size_t i = it->index;
        it->index += 1;

        if (metadata_used(it->hs->metadata[i])) {
            *next = (SetEntry){
                .key_ptr = ptr_offset(it->hs->header->keys, i * it->hs->header->key_size),
            };
            return true;
        }
    }

    return false;
}
