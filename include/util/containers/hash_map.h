#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t Hash;

static const size_t HASH_MAP_MAX_LOAD_PERCENTAGE = 80;
static const size_t HASH_MAP_MINIMUM_CAPACITY    = 8;

// Metadata for a slot. It can be in three states: empty, used or
// tombstone. Tombstones indicate that an entry was previously used.
#pragma pack(push, 1)
typedef struct Metadata {
    uint8_t fingerprint : 7;
    uint8_t used        : 1;
} Metadata;
#pragma pack(pop)

typedef enum {
    FP_OPEN      = 0,
    FP_TOMBSTONE = 1,
} Fingerprint;

static const uint8_t  FINGERPRINT_MASK        = 0x7F;
static const Metadata METADATA_SLOT_OPEN      = {FP_OPEN, 0};
static const Metadata METADATA_SLOT_TOMBSTONE = {FP_TOMBSTONE, 0};

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

static inline void metadata_clear(Metadata* m) {
    *m = METADATA_SLOT_OPEN;
}

static inline void metadata_remove(Metadata* m) {
    *m = METADATA_SLOT_TOMBSTONE;
}

// Only the 7 most significant bits of the result are relevant
static inline uint8_t take_fingerprint(Hash hash) {
    return FINGERPRINT_MASK & (hash >> (64 - 7));
}

// A pair of mutable pointers into the table.
//
// While you can technically change the key, it will invalidate so much of the
// backing metadata that the table will be practically unusable.
typedef struct {
    void* key_ptr;
    void* value_ptr;
} MapEntry;

// A pointer into the map either that mutates the map directly.
//
// Must be used for `get_or_put` calls to set the value.
typedef struct {
    void* key_ptr;
    void* value_ptr;
    bool  found_existing;
} MapGetOrPutResult;

typedef struct {
    void*  keys;
    size_t key_size;
    size_t key_align;

    void*  values;
    size_t value_size;
    size_t value_align;

    size_t capacity;
} MapHeader;

// A HashMap based on open addressing and linear probing. This is not thread-safe.
//
// Operations that grow the map or rehash its entries invalidate all pointers.
typedef struct {
    size_t     size;
    size_t     available;
    void*      buffer;
    MapHeader* header;
    Metadata*  metadata;

    Hash (*hash)(const void*);
    int (*compare)(const void*, const void*);
} HashMap;

// A non-owning iterator. Invalid if the underlying map is freed.
// An iterator is invalidated if it modifies the map during iteration.
//
// HashMap growths invalidate pointers.
typedef struct {
    HashMap* hm;
    size_t   index;
} HashMapIterator;

// Creates a HashMap with the given properties.
//
// For use as a HashSet, use a value size and alignment of 1.
//
// `compare` must be a function pointer for keys such that:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// `hash` is a user defined function pointer for hashing keys.
//
// Heavily inspired by Zig's unmanaged HashMap.
bool hash_map_init(HashMap* hm,
                   size_t   capacity,
                   size_t   key_size,
                   size_t   key_align,
                   size_t   value_size,
                   size_t   value_align,
                   Hash (*hash)(const void*),
                   int (*compare)(const void*, const void*));
void hash_map_deinit(HashMap* hm);

size_t hash_map_capacity(const HashMap* hm);
size_t hash_map_count(const HashMap* hm);

void hash_map_clear_retaining_capacity(HashMap* hm);
bool hash_map_ensure_total_capacity(HashMap* hm, size_t new_size);
bool hash_map_ensure_unused_capacity(HashMap* hm, size_t additional_size);

// Rehash the map, in-place.
//
// Over time, due to the current tombstone-based implementation, a
// HashMap could become fragmented due to the buildup of tombstone
// entries that causes a performance degradation due to excessive
// probing. The kind of pattern that might cause this is a long-lived
// HashMap with repeated inserts and deletes.
//
// After this function is called, there will be no tombstones in
// the HashMap, each of the entries is rehashed and any existing
// key/value pointers into the HashMap are invalidated.
void hash_map_rehash(HashMap* hm);

// Inserts an entry into the map, assuming it is not present and no growth is needed.
void hash_map_put_assume_capacity_no_clobber(HashMap* hm, const void* key, const void* value);
bool hash_map_put_no_clobber(HashMap* hm, const void* key, const void* value);

MapGetOrPutResult hash_map_get_or_put_assume_capacity(HashMap* hm, const void* key);
bool              hash_map_get_or_put(HashMap* hm, const void* key, MapGetOrPutResult* result);
void              hash_map_put_assume_capacity(HashMap* hm, const void* key, const void* value);
bool              hash_map_put(HashMap* hm, const void* key, const void* value);

bool  hash_map_contains(const HashMap* hm, const void* key);
bool  hash_map_get_index(const HashMap* hm, const void* key, size_t* index);
bool  hash_map_get_value(const HashMap* hm, const void* key, void* value);
void* hash_map_get_value_ptr(HashMap* hm, const void* key);

// Gets the entry corresponding to the provided key. The returned data is owned by the map.
bool hash_map_get_entry(HashMap* hm, const void* key, MapEntry* e);

bool hash_map_remove(HashMap* hm, const void* key);

HashMapIterator hash_map_iterator_init(HashMap* hm);
bool            hash_map_iterator_has_next(HashMapIterator* it, MapEntry* next);
