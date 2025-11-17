#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/hash.h"
#include "util/math.h"
#include "util/mem.h"
#include "util/status.h"

static_assert(sizeof(Metadata) == 1, "Metadata must be a single byte in size.");
static_assert(alignof(Metadata) == 1, "Metadata must be a single byte alignment.");

MAX_FN(size_t, size_t)

// Determines the new capacity based on the current size.
static inline size_t _hash_map_capacity_for_size(size_t size) {
    size_t new_cap = (size * 100) / HASH_MAP_MAX_LOAD_PERCENTAGE + 1;
    return ceil_power_of_two_size(new_cap);
}

// Grows the map to the new capacity, rehashing along the way.
static inline TRY_STATUS _hash_map_grow(HashMap* hm, size_t new_capacity) {
    assert(hm);
    new_capacity = max_size_t(2, new_capacity, HASH_MAP_MINIMUM_CAPACITY);
    assert(new_capacity > hm->header->capacity);
    assert(is_power_of_two(new_capacity));

    HashMap map;
    PROPAGATE_IF_ERROR(hash_map_init(&map,
                                     new_capacity,
                                     hm->header->key_size,
                                     hm->header->key_align,
                                     hm->header->value_size,
                                     hm->header->value_align,
                                     hm->hash,
                                     hm->compare));

    // Append all data to the map without checking sizes
    if (hm->size != 0) {
        const size_t old_capacity = hm->header->capacity;
        for (size_t i = 0; i < old_capacity; i++) {
            if (!metadata_used(hm->metadata[i])) {
                continue;
            }

            const void* key   = ptr_offset(hm->header->keys, i * hm->header->key_size);
            const void* value = ptr_offset(hm->header->values, i * hm->header->value_size);
            hash_map_put_assume_capacity_no_clobber(&map, key, value);

            if (map.size == hm->size) {
                break;
            }
        }
    }
    map.available = (new_capacity * HASH_MAP_MAX_LOAD_PERCENTAGE) / 100 - map.size;

    hash_map_deinit(hm);
    *hm = map;
    return SUCCESS;
}

// Only grows if the requested count exceeds the current number of available slots.
static inline TRY_STATUS _hash_map_grow_if_needed(HashMap* hm, size_t new_count) {
    assert(hm);
    const size_t max_load = (hm->header->capacity * HASH_MAP_MAX_LOAD_PERCENTAGE) / 100;
    if (hm->size + new_count <= max_load) {
        return SUCCESS;
    }

    const size_t desired = _hash_map_capacity_for_size(hm->size + new_count);
    PROPAGATE_IF_ERROR(_hash_map_grow(hm, desired));
    return SUCCESS;
}

static inline void _hash_map_init_metadatas(HashMap* hm) {
    assert(hm);
    assert(METADATA_SLOT_OPEN.fingerprint == 0);
    assert(METADATA_SLOT_OPEN.used == 0);
    memset(hm->metadata, 0, sizeof(Metadata) * hm->header->capacity);
}

TRY_STATUS hash_map_init_allocator(HashMap* hm,
                                   size_t   capacity,
                                   size_t   key_size,
                                   size_t   key_align,
                                   size_t   value_size,
                                   size_t   value_align,
                                   Hash (*hash)(const void*),
                                   int (*compare)(const void*, const void*),
                                   Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);
    if (!hm || !hash || !compare) {
        return NULL_PARAMETER;
    } else if (key_size == 0 || value_size == 0) {
        return ZERO_ITEM_SIZE;
    } else if (key_align == 0 || value_align == 0) {
        return ZERO_ITEM_ALIGN;
    }

    capacity = capacity < HASH_MAP_MINIMUM_CAPACITY ? HASH_MAP_MINIMUM_CAPACITY : capacity;
    if (!is_power_of_two(capacity)) {
        capacity = ceil_power_of_two_size(capacity);
    }

    const size_t header_size   = sizeof(MapHeader);
    const size_t metadata_size = sizeof(Metadata) * capacity;
    const size_t keys_size     = key_size * capacity;
    const size_t values_size   = value_size * capacity;

    // Safety checks before allocating for overflow guard
    if (capacity > SIZE_MAX / key_size) {
        return SIZE_OVERFLOW;
    } else if (capacity > SIZE_MAX / value_size) {
        return SIZE_OVERFLOW;
    } else if (metadata_size > SIZE_MAX - header_size) {
        return SIZE_OVERFLOW;
    }

    const size_t total_size =
        header_size + metadata_size + (keys_size + key_align - 1) + (values_size + value_align - 1);
    void* buffer = allocator.continuous_alloc(1, total_size);
    if (!buffer) {
        return ALLOCATION_FAILED;
    }

    // Unpack the allocated block of memory
    uintptr_t ptr = (uintptr_t)buffer;

    MapHeader* header = (MapHeader*)ptr;
    ptr               = (uintptr_t)ptr_offset((void*)ptr, sizeof(MapHeader));

    Metadata* metadata = (Metadata*)ptr;
    ptr                = (uintptr_t)ptr_offset((void*)ptr, sizeof(Metadata) * capacity);

    void* keys = align_ptr((void*)ptr, key_align);
    ptr        = (uintptr_t)ptr_offset(keys, key_size * capacity);

    void* values = align_ptr((void*)ptr, value_align);
    ptr          = (uintptr_t)ptr_offset(values, value_size * capacity);

    assert(((uintptr_t)keys % key_align) == 0);
    assert(((uintptr_t)values % value_align) == 0);

    // Fill header info
    *header = (MapHeader){
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
        .size      = 0,
        .available = capacity,
        .hash      = hash,
        .compare   = compare,
        .allocator = allocator,
    };

    _hash_map_init_metadatas(hm);
    return SUCCESS;
}

TRY_STATUS hash_map_init(HashMap* hm,
                         size_t   capacity,
                         size_t   key_size,
                         size_t   key_align,
                         size_t   value_size,
                         size_t   value_align,
                         Hash (*hash)(const void*),
                         int (*compare)(const void*, const void*)) {
    return hash_map_init_allocator(hm,
                                   capacity,
                                   key_size,
                                   key_align,
                                   value_size,
                                   value_align,
                                   hash,
                                   compare,
                                   standard_allocator);
}

void hash_map_deinit(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return;
    }
    ASSERT_ALLOCATOR(hm->allocator);

    hm->allocator.free_alloc(hm->buffer);
    hm->buffer    = NULL;
    hm->header    = NULL;
    hm->metadata  = NULL;
    hm->size      = 0;
    hm->available = 0;
}

size_t hash_map_capacity(const HashMap* hm) {
    assert(hm && hm->buffer);
    return hm->header->capacity;
}

size_t hash_map_count(const HashMap* hm) {
    assert(hm && hm->buffer);
    return hm->size;
}

void hash_map_clear_retaining_capacity(HashMap* hm) {
    _hash_map_init_metadatas(hm);
    hm->size      = 0;
    hm->available = (hm->header->capacity * HASH_MAP_MAX_LOAD_PERCENTAGE) / 100;
}

TRY_STATUS hash_map_ensure_total_capacity(HashMap* hm, size_t new_size) {
    if (!hm || !hm->buffer) {
        return NULL_PARAMETER;
    }

    if (new_size > hm->size) {
        PROPAGATE_IF_ERROR_IS(_hash_map_grow_if_needed(hm, new_size - hm->size), ALLOCATION_FAILED);
    }
    return SUCCESS;
}

TRY_STATUS hash_map_ensure_unused_capacity(HashMap* hm, size_t additional_size) {
    return hash_map_ensure_total_capacity(hm, hm->size + additional_size);
}

void hash_map_rehash(HashMap* hm) {
    assert(hm && hm->buffer);

    const size_t capacity = hm->header->capacity;
    const size_t mask     = hm->header->capacity - 1;

    Metadata* metadata   = hm->metadata;
    void*     keys_ptr   = hm->header->keys;
    void*     values_ptr = hm->header->values;
    size_t    current    = 0;

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

        const Hash   hash        = hm->hash(ptr_offset(keys_ptr, current * hm->header->key_size));
        const size_t fingerprint = take_fingerprint(hash);
        size_t       probe       = hash & mask;

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

            const void* old_key   = ptr_offset(keys_ptr, current * hm->header->key_size);
            const void* old_value = ptr_offset(values_ptr, current * hm->header->value_size);
            void*       new_key   = ptr_offset(keys_ptr, probe * hm->header->key_size);
            void*       new_value = ptr_offset(values_ptr, probe * hm->header->value_size);

            memcpy(new_key, old_key, hm->header->key_size);
            memcpy(new_value, old_value, hm->header->value_size);

            metadata[current].used = 0;
            assert(metadata_open(metadata[current]));
        } else if (probe == current) {
            metadata_fill(&metadata[probe], fingerprint);
        } else {
            assert(metadata[probe].fingerprint != FP_TOMBSTONE);
            metadata[probe].fingerprint = FP_TOMBSTONE;

            if (metadata_used(metadata[probe])) {
                swap(ptr_offset(keys_ptr, current * hm->header->key_size),
                     ptr_offset(keys_ptr, probe * hm->header->key_size),
                     hm->header->key_size);
                swap(ptr_offset(values_ptr, current * hm->header->value_size),
                     ptr_offset(values_ptr, probe * hm->header->value_size),
                     hm->header->value_size);
                continue;
            } else {
                metadata[probe].used  = 1;
                const void* old_key   = ptr_offset(keys_ptr, current * hm->header->key_size);
                const void* old_value = ptr_offset(values_ptr, current * hm->header->value_size);
                void*       new_key   = ptr_offset(keys_ptr, probe * hm->header->key_size);
                void*       new_value = ptr_offset(values_ptr, probe * hm->header->value_size);

                memcpy(new_key, old_key, hm->header->key_size);
                memcpy(new_value, old_value, hm->header->value_size);
                metadata_clear(&metadata[current]);
            }
        }

        current += 1;
    }

    // Convert graveyard into real fingerprints
    for (size_t i = 0; i < capacity; i++) {
        if (metadata[i].fingerprint == FP_TOMBSTONE) {
            // This was a slot > current that was processed
            assert(metadata_used(metadata[i]));
            const Hash hash         = hm->hash(ptr_offset(keys_ptr, i * hm->header->key_size));
            metadata[i].fingerprint = take_fingerprint(hash);
        } else {
            // This was either:
            // 1. A slot <= current that was processed (has correct fp)
            // 2. An empty slot (fp=FP_OPEN, used=0)
            assert(metadata_used(metadata[i]) || metadata_open(metadata[i]));
        }
    }

    hm->available = (hm->header->capacity * HASH_MAP_MAX_LOAD_PERCENTAGE) / 100 - hm->size;
}

void hash_map_put_assume_capacity_no_clobber(HashMap* hm, const void* key, const void* value) {
    assert(hm && hm->buffer && key && value);
    assert(!hash_map_contains(hm, key));

    const Hash   hash = hm->hash(key);
    const size_t mask = hm->header->capacity - 1;

    size_t probe = hash & mask;

    Metadata* metadata = hm->metadata;

    while (metadata_used(metadata[probe])) {
        probe = (probe + 1) & mask;
    }

    assert(hm->available > 0);
    hm->available -= 1;

    const uint8_t fingerprint = take_fingerprint(hash);
    metadata_fill(&hm->metadata[probe], fingerprint);

    void* key_slot   = ptr_offset(hm->header->keys, probe * hm->header->key_size);
    void* value_slot = ptr_offset(hm->header->values, probe * hm->header->value_size);

    memcpy(key_slot, key, hm->header->key_size);
    memcpy(value_slot, value, hm->header->value_size);

    hm->size += 1;
}

TRY_STATUS hash_map_put_no_clobber(HashMap* hm, const void* key, const void* value) {
    PROPAGATE_IF_ERROR_IS(_hash_map_grow_if_needed(hm, 1), ALLOCATION_FAILED);

    hash_map_put_assume_capacity_no_clobber(hm, key, value);
    return SUCCESS;
}

// The data in the returned result is garbage if an existing key was not found.
MapGetOrPutResult hash_map_get_or_put_assume_capacity(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);

    const Hash    hash        = hm->hash(key);
    const size_t  mask        = hm->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hm->header->capacity;
    size_t probe = hash & mask;

    size_t    first_tombstone_idx = limit;
    Metadata* metadata            = hm->metadata;

    Metadata m = metadata[probe];
    while (!metadata_open(m) && limit != 0) {
        if (metadata_used(m) && m.fingerprint == fingerprint) {
            void* test_key = ptr_offset(hm->header->keys, probe * hm->header->key_size);

            if (hm->compare(key, test_key) == 0) {
                return (MapGetOrPutResult){
                    .key_ptr   = test_key,
                    .value_ptr = ptr_offset(hm->header->values, probe * hm->header->value_size),
                    .found_existing = true,
                };
            }
        } else if (first_tombstone_idx == hm->header->capacity && metadata_tombstone(m)) {
            first_tombstone_idx = probe;
        }

        limit -= 1;
        probe = (probe + 1) & mask;
        m     = metadata[probe];
    }

    // It's cheaper to lower probing distance after deletions by recycling a tombstone
    if (first_tombstone_idx < hm->header->capacity) {
        probe = first_tombstone_idx;
    }
    hm->available -= 1;

    metadata_fill(&hm->metadata[probe], fingerprint);
    hm->size += 1;

    return (MapGetOrPutResult){
        .key_ptr        = ptr_offset(hm->header->keys, probe * hm->header->key_size),
        .value_ptr      = ptr_offset(hm->header->values, probe * hm->header->value_size),
        .found_existing = false,
    };
}

TRY_STATUS hash_map_get_or_put(HashMap* hm, const void* key, MapGetOrPutResult* result) {
    assert(hm && hm->buffer && key);

    // If we fail to grow, still try to find the key
    if (_hash_map_grow_if_needed(hm, 1) == ALLOCATION_FAILED) {
        if (hm->available > 0) {
            *result = hash_map_get_or_put_assume_capacity(hm, key);
            return SUCCESS;
        }

        size_t index;
        PROPAGATE_IF_ERROR(hash_map_get_index(hm, key, &index));

        *result = (MapGetOrPutResult){
            .key_ptr        = ptr_offset(hm->header->keys, index * hm->header->key_size),
            .value_ptr      = ptr_offset(hm->header->values, index * hm->header->value_size),
            .found_existing = true,
        };
        return SUCCESS;
    }

    *result = hash_map_get_or_put_assume_capacity(hm, key);
    return SUCCESS;
}

void hash_map_put_assume_capacity(HashMap* hm, const void* key, const void* value) {
    assert(hm && hm->buffer && key && value);
    MapGetOrPutResult gop = hash_map_get_or_put_assume_capacity(hm, key);

    memcpy(gop.key_ptr, key, hm->header->key_size);
    memcpy(gop.value_ptr, value, hm->header->value_size);
}

TRY_STATUS hash_map_put(HashMap* hm, const void* key, const void* value) {
    assert(hm && hm->buffer && key && value);
    MapGetOrPutResult gop;
    PROPAGATE_IF_ERROR(hash_map_get_or_put(hm, key, &gop));

    memcpy(gop.key_ptr, key, hm->header->key_size);
    memcpy(gop.value_ptr, value, hm->header->value_size);
    return SUCCESS;
}

bool hash_map_contains(const HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    return STATUS_OK(hash_map_get_index(hm, key, NULL));
}

TRY_STATUS hash_map_get_index(const HashMap* hm, const void* key, size_t* index) {
    assert(hm && hm->buffer && key);
    if (hm->size == 0) {
        return EMPTY;
    }

    const Hash    hash        = hm->hash(key);
    const size_t  mask        = hm->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hm->header->capacity;
    size_t probe = hash & mask;

    Metadata* metadata = hm->metadata;

    Metadata m = metadata[probe];
    while (!metadata_open(m) && limit != 0) {
        if (metadata_used(m) && m.fingerprint == fingerprint) {
            const void* test_key = ptr_offset(hm->header->keys, probe * hm->header->key_size);

            if (hm->compare(key, test_key) == 0) {
                if (index) {
                    *index = probe;
                }
                return SUCCESS;
            }
        }

        limit -= 1;
        probe = (probe + 1) & mask;
        m     = metadata[probe];
    }

    return ELEMENT_MISSING;
}

static inline const void* _hash_map_get_value_ptr(const HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    size_t index;
    if (STATUS_ERR(hash_map_get_index(hm, key, &index))) {
        return NULL;
    }

    return ptr_offset(hm->header->values, index * hm->header->value_size);
}

TRY_STATUS hash_map_get_value(const HashMap* hm, const void* key, void* value) {
    assert(hm && hm->buffer && key && value);
    const void* stored = _hash_map_get_value_ptr(hm, key);
    if (stored) {
        memcpy(value, stored, hm->header->value_size);
        return SUCCESS;
    } else {
        return ELEMENT_MISSING;
    }
}

TRY_STATUS hash_map_get_value_ptr(HashMap* hm, const void* key, void** item) {
    assert(hm && hm->buffer && key);
    size_t index;
    PROPAGATE_IF_ERROR(hash_map_get_index(hm, key, &index));

    *item = ptr_offset(hm->header->values, index * hm->header->value_size);
    return SUCCESS;
}

TRY_STATUS hash_map_get_entry(HashMap* hm, const void* key, MapEntry* e) {
    assert(hm && hm->buffer && key);
    size_t index;
    PROPAGATE_IF_ERROR(hash_map_get_index(hm, key, &index));

    *e = (MapEntry){
        .key_ptr   = ptr_offset(hm->header->keys, index * hm->header->key_size),
        .value_ptr = ptr_offset(hm->header->values, index * hm->header->value_size),
    };
    return SUCCESS;
}

TRY_STATUS hash_map_remove(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    size_t index;
    PROPAGATE_IF_ERROR(hash_map_get_index(hm, key, &index));

    metadata_remove(&hm->metadata[index]);
    hm->size -= 1;
    hm->available += 1;
    return SUCCESS;
}

HashMapIterator hash_map_iterator_init(HashMap* hm) {
    assert(hm && hm->buffer);
    return (HashMapIterator){
        .hm    = hm,
        .index = 0,
    };
}

bool hash_map_iterator_has_next(HashMapIterator* it, MapEntry* next) {
    assert(it && it->hm && it->hm->buffer);
    assert(it->index <= it->hm->header->capacity);
    if (it->hm->size == 0) {
        return false;
    }

    const size_t capacity = it->hm->header->capacity;
    while (it->index < capacity) {
        const size_t i = it->index;
        it->index += 1;

        if (metadata_used(it->hm->metadata[i])) {
            *next = (MapEntry){
                .key_ptr   = ptr_offset(it->hm->header->keys, i * it->hm->header->key_size),
                .value_ptr = ptr_offset(it->hm->header->values, i * it->hm->header->value_size),
            };
            return true;
        }
    }

    return false;
}
