#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint64_t Hash;

// Metadata for a slot. It can be in three states: empty, used or
// tombstone. Tombstones indicate that an entry was previously used.
#pragma pack(push, 1)
typedef struct {
    uint8_t fingerprint : 7;
    uint8_t used        : 1;
} Metadata;
#pragma pack(pop)

static_assert(sizeof(Metadata) == 1, "Metadata must be a single byte in size.");

static const uint8_t  FINGERPRINT_MASK        = 0x7F;
static const Metadata METADATA_SLOT_OPEN      = {0, 0};
static const Metadata METADATA_SLOT_TOMBSTONE = {1, 0};

static inline bool metadata_equal(Metadata a, Metadata b) {
    return a.fingerprint == b.fingerprint && a.used == b.used;
}

static inline bool metadata_used(Metadata m) {
    return m.used == 1;
}

static inline bool metadata_tombstone(Metadata m) {
    return metadata_equal(m, METADATA_SLOT_TOMBSTONE);
}

static inline bool metadata_open(Metadata m) {
    return metadata_equal(m, METADATA_SLOT_OPEN);
}

static inline void metadata_fill(Metadata* m, uint8_t fp) {
    m->fingerprint = FINGERPRINT_MASK & fp;
    m->used        = 1;
}

static inline void metadata_remove(Metadata* m) {
    m->fingerprint = METADATA_SLOT_TOMBSTONE.fingerprint;
    m->used        = 0;
}

// Only the first 7 bits of the result are relevant
static inline uint8_t take_fingerprint(Hash hash) {
    return FINGERPRINT_MASK & (hash >> (64 - 7));
}

typedef struct {
    void* key;
    void* value;
} Entry;

typedef struct {
    void*  keys;
    size_t key_size;
    size_t key_align;

    void*  values;
    size_t value_size;
    size_t value_align;

    size_t capacity;
} Header;

// A HashMap based on open addressing and linear probing.
typedef struct {
    size_t    size;
    size_t    available;
    void*     buffer;
    Header*   header;
    Entry*    entries;
    Metadata* metadata;

    Hash (*hash)(const void*);
    int (*compare)(const void*, const void*);
} HashMap;

static const size_t MAX_LOAD_PERCENTAGE = 80;
static const size_t MINIMUM_CAPACITY    = 8;

// Creates a HashMap with the given properties.
//
// `compare` must be a function pointer for keys such that:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// `hash` is a user defined function pointer for hashing keys.
bool hash_map_init(HashMap* hm,
                   size_t   capacity,
                   size_t   key_size,
                   size_t   key_align,
                   size_t   value_size,
                   size_t   value_align,
                   Hash (*hash)(const void*),
                   int (*compare)(const void*, const void*));
void hash_map_deinit(HashMap* hm);
