#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "util/hash_map.h"
#include "util/math.h"
#include "util/mem.h"

static_assert(sizeof(Metadata) == 1, "Metadata must be a single byte in size.");
static_assert(alignof(Metadata) == 1, "Metadata must be a single byte alignment.");

MAX_FN(size_t, size_t)

// Determines the new capacity based on the current size.
static inline size_t _hash_map_capacity_for_size(size_t size) {
    size_t new_cap = (size * 100) / HM_MAX_LOAD_PERCENTAGE + 1;
#ifdef WORD_SIZE_64
    return (size_t)ceil_power_of_two_64(new_cap);
#else
    return (size_t)ceil_power_of_two_32(new_cap);
#endif
}

typedef enum {
    FAILED = -1,
    NOT_NEEDED,
    SUCCESS,
} GrowIfNeededResult;

// Grows the map to the new capacity, rehashing along the way.
static inline bool _hash_map_grow(HashMap* hm, size_t new_capacity) {
    new_capacity = max_size_t(2, new_capacity, HM_MINIMUM_CAPACITY);
    assert(new_capacity > hm->header->capacity);
    assert(is_power_of_two(new_capacity));

    HashMap map;
    if (!hash_map_init(&map,
                       new_capacity,
                       hm->header->key_size,
                       hm->header->key_align,
                       hm->header->value_size,
                       hm->header->value_align,
                       hm->hash,
                       hm->compare)) {
        return false;
    }

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
    map.available = (new_capacity * HM_MAX_LOAD_PERCENTAGE) / 100 - map.size;

    hash_map_deinit(hm);
    *hm = map;
    return true;
}

// Only grows if the requested count exceeds the current number of available slots.
static inline GrowIfNeededResult _hash_map_grow_if_needed(HashMap* hm, size_t new_count) {
    const size_t max_load = (hm->header->capacity * HM_MAX_LOAD_PERCENTAGE) / 100;
    if (hm->size + new_count <= max_load) {
        return NOT_NEEDED;
    }

    const size_t desired = _hash_map_capacity_for_size(hm->size + new_count);
    if (!_hash_map_grow(hm, desired)) {
        return FAILED;
    }
    return SUCCESS;
}

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

    capacity = capacity < HM_MINIMUM_CAPACITY ? HM_MINIMUM_CAPACITY : capacity;

    const size_t header_size   = sizeof(Header);
    const size_t metadata_size = sizeof(Metadata) * capacity;
    const size_t keys_size     = key_size * capacity;
    const size_t values_size   = value_size * capacity;

    // Safety checks before allocating for overflow guard
    if (capacity > SIZE_MAX / key_size) {
        return false;
    } else if (capacity > SIZE_MAX / value_size) {
        return false;
    } else if (metadata_size > SIZE_MAX - header_size) {
        return false;
    }

    const size_t total_size =
        header_size + metadata_size + (keys_size + key_align - 1) + (values_size + value_align - 1);
    void* buffer = calloc(1, total_size);
    if (!buffer) {
        return false;
    }

    // Unpack the allocated block of memory
    uintptr_t ptr = (uintptr_t)buffer;

    Header* header = (Header*)ptr;
    ptr            = (uintptr_t)ptr_offset((void*)ptr, sizeof(Header));

    Metadata* metadata = (Metadata*)ptr;
    ptr                = (uintptr_t)ptr_offset((void*)ptr, sizeof(Metadata) * capacity);

    void* keys = align_ptr((void*)ptr, key_align);
    ptr        = (uintptr_t)ptr_offset(keys, key_size * capacity);

    void* values = align_ptr((void*)ptr, value_align);
    ptr          = (uintptr_t)ptr_offset(values, value_size * capacity);

    assert(((uintptr_t)keys % key_align) == 0);
    assert(((uintptr_t)values % value_align) == 0);

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
    hm->size      = 0;
    hm->available = 0;
}

size_t hash_map_capacity(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return 0;
    }

    return hm->header->capacity;
}

size_t hash_map_count(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return 0;
    }

    return hm->size;
}

void* hash_map_keys(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return NULL;
    }

    return hm->header->keys;
}

void* hash_map_values(HashMap* hm) {
    if (!hm || !hm->buffer) {
        return NULL;
    }

    return hm->header->values;
}

bool hash_map_ensure_total_capacity(HashMap* hm, size_t new_size) {
    if (!hm || !hm->buffer) {
        return false;
    }

    if (new_size > hm->size) {
        const GrowIfNeededResult result = _hash_map_grow_if_needed(hm, new_size - hm->size);
        if (result == FAILED) {
            return false;
        }
    }
    return true;
}

bool hash_map_ensure_unused_capacity(HashMap* hm, size_t additional_size) {
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
            assert(metadata[probe].fingerprint == FP_TOMBSTONE);
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

// The data in the returned result is garbage if an existing key was not found.
GetOrPutResult hash_map_get_or_put_assume_capacity(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);

    const Hash    hash        = hm->hash(key);
    const size_t  mask        = hm->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hm->header->capacity;
    size_t probe = hash & mask;

    size_t    first_tombstone_idx = limit;
    Metadata* metadata            = hm->metadata;

    while (limit != 0) {
        Metadata m = metadata[probe];
        if (metadata_open(m)) {
            break;
        }

        if (metadata_used(m) && m.fingerprint == fingerprint) {
            void* test_key = ptr_offset(hm->header->keys, probe * hm->header->key_size);

            if (hm->compare(key, test_key) == 0) {
                return (GetOrPutResult){
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
    }

    // It's cheaper to lower probing distance after deletions by recycling a tombstone
    if (first_tombstone_idx < hm->header->capacity) {
        probe = first_tombstone_idx;
    }
    hm->available -= 1;

    metadata_fill(&hm->metadata[probe], fingerprint);
    hm->size += 1;

    return (GetOrPutResult){
        .key_ptr        = ptr_offset(hm->header->keys, probe * hm->header->key_size),
        .value_ptr      = ptr_offset(hm->header->values, probe * hm->header->value_size),
        .found_existing = false,
    };
}

bool hash_map_get_or_put(HashMap* hm, const void* key, GetOrPutResult* result) {
    assert(hm && hm->buffer && key);
    const GrowIfNeededResult grow_res = _hash_map_grow_if_needed(hm, 1);

    // If we fail to grow, still try to find the key
    if (grow_res == FAILED) {
        if (hm->available > 0) {
            *result = hash_map_get_or_put_assume_capacity(hm, key);
            return true;
        }

        size_t index;
        if (!hash_map_get_index(hm, key, &index)) {
            return false;
        }

        *result = (GetOrPutResult){
            .key_ptr        = ptr_offset(hm->header->keys, index * hm->header->key_size),
            .value_ptr      = ptr_offset(hm->header->values, index * hm->header->value_size),
            .found_existing = true,
        };
        return true;
    }

    *result = hash_map_get_or_put_assume_capacity(hm, key);
    return true;
}

bool hash_map_put(HashMap* hm, const void* key, const void* value) {
    assert(hm && hm->buffer && key && value);
    GetOrPutResult gop;
    if (!hash_map_get_or_put(hm, key, &gop)) {
        return false;
    }

    memcpy(gop.value_ptr, value, hm->header->value_size);
    return true;
}

bool hash_map_contains(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    return hash_map_get_index(hm, key, NULL);
}

bool hash_map_get_index(HashMap* hm, const void* key, size_t* index) {
    assert(hm && hm->buffer && key);
    if (hm->size == 0) {
        return false;
    }

    const Hash    hash        = hm->hash(key);
    const size_t  mask        = hm->header->capacity - 1;
    const uint8_t fingerprint = take_fingerprint(hash);

    size_t limit = hm->header->capacity;
    size_t probe = hash & mask;

    Metadata* metadata = hm->metadata;

    while (limit != 0) {
        Metadata m = metadata[probe];
        if (metadata_open(m)) {
            return false;
        }

        if (metadata_used(m) && m.fingerprint == fingerprint) {
            const void* test_key = ptr_offset(hm->header->keys, probe * hm->header->key_size);

            if (hm->compare(key, test_key) == 0) {
                if (index) {
                    *index = probe;
                }
                return true;
            }
        }

        limit -= 1;
        probe = (probe + 1) & mask;
    }

    return false;
}

bool hash_map_get_value(HashMap* hm, const void* key, void* value) {
    assert(hm && hm->buffer && key && value);
    const void* stored = hash_map_get_value_ptr(hm, key);
    if (stored) {
        memcpy(value, stored, hm->header->value_size);
        return true;
    } else {
        return false;
    }
}

void* hash_map_get_value_ptr(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    size_t index;
    if (!hash_map_get_index(hm, key, &index)) {
        return NULL;
    }

    return ptr_offset(hm->header->values, index * hm->header->value_size);
}

bool hash_map_get_entry(HashMap* hm, const void* key, Entry* e) {
    assert(hm && hm->buffer && key);
    size_t index;
    if (!hash_map_get_index(hm, key, &index)) {
        return false;
    }

    *e = (Entry){
        .key   = ptr_offset(hm->header->keys, index * hm->header->key_size),
        .value = ptr_offset(hm->header->values, index * hm->header->value_size),
    };
    return true;
}

bool hash_map_remove(HashMap* hm, const void* key) {
    assert(hm && hm->buffer && key);
    size_t index;
    if (!hash_map_get_index(hm, key, &index)) {
        return false;
    }

    metadata_remove(&hm->metadata[index]);
    hm->size -= 1;
    hm->available += 1;
    return true;
}

HashMapIterator hash_map_iterator_init(HashMap* hm) {
    assert(hm && hm->buffer);
    return (HashMapIterator){
        .hm    = hm,
        .index = 0,
    };
}

bool hash_map_iterator_next(HashMapIterator* it, Entry* e) {
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
            *e = (Entry){
                .key   = ptr_offset(it->hm->header->keys, i * it->hm->header->key_size),
                .value = ptr_offset(it->hm->header->values, i * it->hm->header->value_size),
            };
            return true;
        }
    }

    return false;
}
