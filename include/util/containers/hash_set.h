#ifndef HASH_SET_H
#define HASH_SET_H

#include <stddef.h>
#include <stdint.h>

#include "util/hash.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct Metadata Metadata;

static const size_t HASH_SET_MAX_LOAD_PERCENTAGE = 80;
static const size_t HASH_SET_MINIMUM_CAPACITY    = 8;

// A mutable pointers into the set.
//
// The mutability of this is required by the implementation, but external use is undefined behavior
// as it invalidates the set's invariants.
typedef struct {
    void* key_ptr;
} SetEntry;

// A pointer into the map either that mutates the map directly.
//
// Must be used for `get_or_put` calls to set the value.
typedef struct {
    void* key_ptr;
    bool  found_existing;
} SetGetOrPutResult;

typedef struct {
    void*  keys;
    size_t key_size;
    size_t key_align;

    size_t capacity;
} SetHeader;

// A HashSet based on open addressing and linear probing. This is not thread-safe.
// While you can use a HashSet with an arbitrary value, this is more memory efficient.
//
// Operations that grow the map or rehash its entries invalidate all pointers.
typedef struct HashSet {
    size_t     size;
    size_t     available;
    void*      buffer;
    SetHeader* header;
    Metadata*  metadata;

    Hash (*hash)(const void*);
    int (*compare)(const void*, const void*);

    Allocator allocator;
} HashSet;

// A non-owning iterator. Invalid if the underlying set is freed.
// An iterator is invalidated if it modifies the set during iteration.
typedef struct {
    HashSet* hs;
    size_t   index;
} HashSetIterator;

// Creates a HashSet with the given properties.
//
// `compare` must be a function pointer for keys such that:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// `hash` is a user defined function pointer for hashing keys.
//
// Stripped implementation of a map.
[[nodiscard]] Status hash_set_init_allocator(HashSet* hs,
                                         size_t   capacity,
                                         size_t   key_size,
                                         size_t   key_align,
                                         Hash (*hash)(const void*),
                                         int (*compare)(const void*, const void*),
                                         Allocator allocator);

// Creates a HashSet with the given properties.
//
// `compare` must be a function pointer for keys such that:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// `hash` is a user defined function pointer for hashing keys.
//
// Stripped implementation of a map.
[[nodiscard]] Status hash_set_init(HashSet* hs,
                               size_t   capacity,
                               size_t   key_size,
                               size_t   key_align,
                               Hash (*hash)(const void*),
                               int (*compare)(const void*, const void*));
void             hash_set_deinit(HashSet* hs);

size_t hash_set_capacity(const HashSet* hs);
size_t hash_set_count(const HashSet* hs);

void             hash_set_clear_retaining_capacity(HashSet* hs);
[[nodiscard]] Status hash_set_ensure_total_capacity(HashSet* hs, size_t new_size);
[[nodiscard]] Status hash_set_ensure_unused_capacity(HashSet* hs, size_t additional_size);

// Rehash the map, in-place.
//
// Over time, due to the current tombstone-based implementation, a
// HashSet could become fragmented due to the buildup of tombstone
// entries that causes a performance degradation due to excessive
// probing. The kind of pattern that might cause this is a long-lived
// HashSet with repeated inserts and deletes.
//
// After this function is called, there will be no tombstones in
// the HashSet, each of the entries is rehashed and any existing
// key/value pointers into the HashSet are invalidated.
void hash_set_rehash(HashSet* hs);

// Inserts an entry into the map, assuming it is not present and no growth is needed.
void             hash_set_put_assume_capacity_no_clobber(HashSet* hs, const void* key);
[[nodiscard]] Status hash_set_put_no_clobber(HashSet* hs, const void* key);

SetGetOrPutResult hash_set_get_or_put_assume_capacity(HashSet* hs, const void* key);
[[nodiscard]] Status  hash_set_get_or_put(HashSet* hs, const void* key, SetGetOrPutResult* result);
void              hash_set_put_assume_capacity(HashSet* hs, const void* key);
[[nodiscard]] Status  hash_set_put(HashSet* hs, const void* key);

bool             hash_set_contains(const HashSet* hs, const void* key);
[[nodiscard]] Status hash_set_get_index(const HashSet* hs, const void* key, size_t* index);

// Gets the entry corresponding to the provided key. The returned data is owned by the map.
[[nodiscard]] Status hash_set_get_entry(HashSet* hs, const void* key, SetEntry* e);

[[nodiscard]] Status hash_set_remove(HashSet* hs, const void* key);

HashSetIterator hash_set_iterator_init(HashSet* hs);
bool            hash_set_iterator_has_next(HashSetIterator* it, SetEntry* next);

#endif
