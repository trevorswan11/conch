#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

extern "C" {
#include "util/containers/hash_map.h"
#include "util/hash.h"
#include "util/mem.h"
#include "util/status.h"
}

TEST_CASE("Metadata helpers") {
    Metadata a, b;

    metadata_clear(&a);
    REQUIRE(metadata_open(a));
    metadata_remove(&b);
    REQUIRE(metadata_tombstone(b));

    metadata_fill(&a, 20);
    REQUIRE(a.fingerprint == 20);
    REQUIRE(metadata_used(a));
    metadata_fill(&b, 255);
    REQUIRE(b.fingerprint == 127);

    REQUIRE(take_fingerprint(300) == 0);
    REQUIRE(take_fingerprint(0xFFFFFFFFFFFFFFF) == 7);
    REQUIRE(take_fingerprint(0xAFFF5FFFFFFFFFFF) == 87);
    REQUIRE(take_fingerprint(0xFFFFFFFFFFFFFFFF) == 127);
}

HASH_INTEGER_FN(uint16_t)
COMPARE_INTEGER_FN(uint16_t)

TEST_CASE("Malformed map usage") {
    using K = uint16_t;
    using V = uint32_t;

    HashMap hm;

    // Null pointers should not allow initialization
    REQUIRE(hash_map_init(NULL,
                          10,
                          sizeof(K),
                          alignof(K),
                          sizeof(V),
                          alignof(V),
                          hash_uint16_t_u,
                          compare_uint16_t) == Status::NULL_PARAMETER);
    REQUIRE(hash_map_init(
                &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), NULL, compare_uint16_t) ==
            Status::NULL_PARAMETER);
    REQUIRE(hash_map_init(
                &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, NULL) ==
            Status::NULL_PARAMETER);

    // Zero size/alignment is illegal
    REQUIRE(hash_map_init(
                &hm, 10, 0, alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_SIZE);
    REQUIRE(hash_map_init(
                &hm, 10, sizeof(K), 0, sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_ALIGN);
    REQUIRE(hash_map_init(
                &hm, 10, sizeof(K), alignof(K), 0, alignof(V), hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_SIZE);
    REQUIRE(hash_map_init(
                &hm, 10, sizeof(K), alignof(K), sizeof(V), 0, hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_ALIGN);

    // Check capacity overflow safety guard
    REQUIRE(hash_map_init(&hm,
                          SIZE_MAX / 2,
                          3,
                          alignof(K),
                          sizeof(V),
                          alignof(V),
                          hash_uint16_t_u,
                          compare_uint16_t) == Status::SIZE_OVERFLOW);
    REQUIRE(hash_map_init(&hm,
                          SIZE_MAX / 8,
                          sizeof(K),
                          alignof(K),
                          9,
                          alignof(V),
                          hash_uint16_t_u,
                          compare_uint16_t) == Status::SIZE_OVERFLOW);

    REQUIRE(hash_map_ensure_total_capacity(NULL, 10) == Status::NULL_PARAMETER);
    hash_map_deinit(NULL);

    hm.buffer = NULL;
    REQUIRE(hash_map_ensure_total_capacity(&hm, 10) == Status::NULL_PARAMETER);
    hash_map_deinit(&hm);
}

TEST_CASE("Init map") {
    using K = uint16_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t)));
    REQUIRE(hm.size == 0);
    REQUIRE(hm.available == 16);

    REQUIRE(hm.header->capacity == 16);
    REQUIRE(hash_map_capacity(&hm) == 16);
    REQUIRE(hm.header->key_size == sizeof(K));
    REQUIRE(hm.header->key_align == alignof(K));
    REQUIRE(hm.header->value_size == sizeof(V));
    REQUIRE(hm.header->value_align == alignof(V));

    for (size_t i = 0; i < hm.header->capacity; i++) {
        REQUIRE(metadata_equal(hm.metadata[i], METADATA_SLOT_OPEN));
    }

    hash_map_deinit(&hm);

    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 1, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t)));
    REQUIRE(hash_map_capacity(&hm) == HASH_MAP_MINIMUM_CAPACITY);
    hash_map_deinit(&hm);
}

HASH_INTEGER_FN(uint32_t)
COMPARE_INTEGER_FN(uint32_t)

TEST_CASE("Basic map usage") {
    using K = uint32_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_u, compare_uint32_t)));
    const size_t count = 5;

    V load_total = 0;
    for (V i = 0; i < count; i++) {
        REQUIRE(STATUS_OK(hash_map_put(&hm, &i, &i)));
        load_total += i;
    }
    REQUIRE(hash_map_count(&hm) == 5);

    V               internal_total = 0;
    HashMapIterator it             = hash_map_iterator_init(&hm);

    MapEntry e;
    while (hash_map_iterator_has_next(&it, &e)) {
        internal_total += *(K*)e.key_ptr;
    }
    REQUIRE(load_total == internal_total);

    internal_total = 0;
    for (V i = 0; i < count; i++) {
        V val;
        REQUIRE(STATUS_OK(hash_map_get_value(&hm, &i, &val)));
        REQUIRE(i == val);
        internal_total += val;
    }
    REQUIRE(load_total == internal_total);

    hash_map_deinit(&hm);
}

COMPARE_INTEGER_FN(int32_t);

TEST_CASE("Ensure total map capacity") {
    using K = int32_t;
    using V = int32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_s, compare_int32_t)));

    REQUIRE(STATUS_OK(hash_map_ensure_total_capacity(&hm, 20)));
    const size_t initial_capacity = hash_map_capacity(&hm);
    REQUIRE(initial_capacity >= 20);

    for (V i = 0; i < 20; i++) {
        MapGetOrPutResult result;
        REQUIRE(STATUS_OK(hash_map_get_or_put(&hm, &i, &result)));
        REQUIRE_FALSE(result.found_existing);
    }
    REQUIRE(initial_capacity == hash_map_capacity(&hm));

    hash_map_deinit(&hm);
}

HASH_INTEGER_FN(uint64_t)
COMPARE_INTEGER_FN(uint64_t)

TEST_CASE("Ensure unused map capacity") {
    using K = int64_t;
    using V = int64_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint64_t_u, compare_uint64_t)));

    REQUIRE(STATUS_OK(hash_map_ensure_unused_capacity(&hm, 32)));
    const size_t capacity = hash_map_capacity(&hm);
    REQUIRE(STATUS_OK(hash_map_ensure_unused_capacity(&hm, 32)));
    REQUIRE(capacity == hash_map_capacity(&hm));

    hash_map_deinit(&hm);
}

TEST_CASE("Ensure unused map capacity with tombstones") {
    using K = int32_t;
    using V = int32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_s, compare_int32_t)));

    for (V i = 0; i < 100; i++) {
        REQUIRE(STATUS_OK(hash_map_ensure_unused_capacity(&hm, 1)));
        hash_map_put_assume_capacity(&hm, &i, &i);
        REQUIRE(STATUS_OK(hash_map_remove(&hm, &i)));
    }

    hash_map_deinit(&hm);
}

TEST_CASE("Clear retaining map capacity") {
    using K = Slice;
    using V = int32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_slice, compare_int32_t)));
    hash_map_clear_retaining_capacity(&hm);

    const char* str1 = "Hello";
    K           key1 = slice_from_str_z(str1);
    V           val1 = 1;
    REQUIRE(STATUS_OK(hash_map_put(&hm, &key1, &val1)));
    REQUIRE(hash_map_count(&hm) == 1);

    hash_map_clear_retaining_capacity(&hm);
    hash_map_put_assume_capacity(&hm, &key1, &val1);
    V out_val1;
    REQUIRE(STATUS_OK(hash_map_get_value(&hm, &key1, &out_val1)));
    REQUIRE(out_val1 == 1);
    REQUIRE(hash_map_count(&hm) == 1);

    const size_t capacity = hash_map_capacity(&hm);
    REQUIRE(capacity > 0);

    hash_map_clear_retaining_capacity(&hm);
    hash_map_clear_retaining_capacity(&hm);

    REQUIRE(hash_map_count(&hm) == 0);
    REQUIRE(hash_map_capacity(&hm) == capacity);
    REQUIRE_FALSE(hash_map_contains(&hm, &key1));

    hash_map_deinit(&hm);
}

TEST_CASE("Grow map") {
    using K = uint32_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_u, compare_uint32_t)));

    const size_t grow_to = 12456;

    for (size_t i = 0; i < grow_to; i++) {
        REQUIRE(STATUS_OK(hash_map_put(&hm, &i, &i)));
    }
    REQUIRE(hash_map_count(&hm) == grow_to);

    HashMapIterator it = hash_map_iterator_init(&hm);
    MapEntry        e;
    size_t          total = 0;
    while (hash_map_iterator_has_next(&it, &e)) {
        REQUIRE(*(K*)e.key_ptr == *(V*)e.value_ptr);
        total += 1;
    }
    REQUIRE(total == grow_to);

    for (size_t i = 0; i < grow_to; i++) {
        V out_val;
        REQUIRE(STATUS_OK(hash_map_get_value(&hm, &i, &out_val)));
        REQUIRE(out_val == i);
    }

    hash_map_deinit(&hm);
}

TEST_CASE("Rehash map") {
    using K = uint32_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_u, compare_uint32_t)));

    // Add some elements and remove every third to simulate a fragmented map
    const size_t total_count = 6 * 1637;
    for (size_t i = 0; i < total_count; i++) {
        REQUIRE(STATUS_OK(hash_map_put(&hm, &i, &i)));
        if (i % 3 == 0) {
            REQUIRE(STATUS_OK(hash_map_remove(&hm, &i)));
        }
    }

    // Rehash and ensure data was not lost along the way
    hash_map_rehash(&hm);
    REQUIRE(hash_map_count(&hm) == total_count * 2 / 3);
    for (size_t i = 0; i < total_count; i++) {
        V out_val;
        if (i % 3 == 0) {
            REQUIRE(hash_map_get_value(&hm, &i, &out_val) == Status::ELEMENT_MISSING);
        } else {
            REQUIRE(STATUS_OK(hash_map_get_value(&hm, &i, &out_val)));
            REQUIRE(out_val == i);
        }
    }

    hash_map_deinit(&hm);
}

TEST_CASE("Mutable map entry access") {
    using K = uint32_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_u, compare_uint32_t)));

    for (K i = 0; i < 16; i++) {
        REQUIRE(STATUS_OK(hash_map_put(&hm, &i, &i)));
    }

    // Basic by-value value retrieval
    const K query_key_initial = 4;
    V       query_val;
    REQUIRE(STATUS_OK(hash_map_get_value(&hm, &query_key_initial, &query_val)));
    REQUIRE(query_val == 4);

    // Pointer retrieval of value
    V* mut_val;
    REQUIRE(STATUS_OK(hash_map_get_value_ptr(&hm, &query_key_initial, (void**)&mut_val)));
    REQUIRE(*mut_val == 4);
    *mut_val = 20;
    REQUIRE(query_val == 4);
    REQUIRE(STATUS_OK(hash_map_get_value(&hm, &query_key_initial, &query_val)));
    REQUIRE(query_val == 20);

    // Entry retrieval for mutable value access (reasonable)
    MapEntry e;
    REQUIRE(STATUS_OK(hash_map_get_entry(&hm, &query_key_initial, &e)));
    REQUIRE(*(K*)e.key_ptr == query_key_initial);
    REQUIRE(*(V*)e.value_ptr == 20);
    *(V*)e.value_ptr = 100;
    REQUIRE(query_val == 20);
    REQUIRE(STATUS_OK(hash_map_get_value(&hm, &query_key_initial, &query_val)));
    REQUIRE(query_val == 100);

    hash_map_deinit(&hm);
}

TEST_CASE("Remove map") {
    using K = uint32_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(STATUS_OK(hash_map_init(
        &hm, 8, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint32_t_u, compare_uint32_t)));

    for (K i = 0; i < 16; i++) {
        REQUIRE(STATUS_OK(hash_map_put(&hm, &i, &i)));
    }

    for (K i = 0; i < 16; i++) {
        if (i % 3 == 0) {
            REQUIRE(STATUS_OK(hash_map_remove(&hm, &i)));
        }
    }
    REQUIRE(hash_map_count(&hm) == 10);

    HashMapIterator it = hash_map_iterator_init(&hm);
    MapEntry        e;
    while (hash_map_iterator_has_next(&it, &e)) {
        K k = *(K*)e.key_ptr;
        V v = *(V*)e.value_ptr;
        REQUIRE(k == v);
        REQUIRE(k % 3 != 0);
    }

    for (K i = 0; i < 16; i++) {
        if (i % 3 == 0) {
            REQUIRE_FALSE(hash_map_contains(&hm, &i));
        } else {
            V out_val;
            REQUIRE(STATUS_OK(hash_map_get_value(&hm, &i, &out_val)));
            REQUIRE(out_val == i);
        }
    }

    hash_map_deinit(&hm);
}
