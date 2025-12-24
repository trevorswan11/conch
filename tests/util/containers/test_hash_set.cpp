#include "catch_amalgamated.hpp"

#include <cstdint>
#include <cstring>

extern "C" {
#include "util/containers/hash_set.h"
}

HASH_INTEGER_FN(uint16_t)
COMPARE_INTEGER_FN(uint16_t)

TEST_CASE("Malformed set usage") {
    using K = uint16_t;

    HashSet hs;

    // Null pointers should not allow initialization
    REQUIRE(hash_set_init(nullptr, 10, sizeof(K), alignof(K), hash_uint16_t_u, compare_uint16_t) ==
            Status::NULL_PARAMETER);
    REQUIRE(hash_set_init(&hs, 10, sizeof(K), alignof(K), nullptr, compare_uint16_t) ==
            Status::NULL_PARAMETER);
    REQUIRE(hash_set_init(&hs, 10, sizeof(K), alignof(K), hash_uint16_t_u, nullptr) ==
            Status::NULL_PARAMETER);

    // Zero size/alignment is illegal
    REQUIRE(hash_set_init(&hs, 10, 0, alignof(K), hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_SIZE);
    REQUIRE(hash_set_init(&hs, 10, sizeof(K), 0, hash_uint16_t_u, compare_uint16_t) ==
            Status::ZERO_ITEM_SIZE);

    // Check capacity overflow safety guard
    REQUIRE(hash_set_init(&hs, SIZE_MAX / 2, 3, alignof(K), hash_uint16_t_u, compare_uint16_t) ==
            Status::SIZE_OVERFLOW);

    // Helpers that check self and buffer initialization
    REQUIRE(hash_set_ensure_total_capacity(nullptr, 10) == Status::NULL_PARAMETER);
    hash_set_deinit(nullptr);

    hs.buffer = nullptr;
    REQUIRE(hash_set_ensure_total_capacity(&hs, 10) == Status::NULL_PARAMETER);
    hash_set_deinit(&hs);
}

TEST_CASE("Set init") {
    using K = uint16_t;

    HashSet hs;
    REQUIRE(STATUS_OK(
        hash_set_init(&hs, 10, sizeof(K), alignof(K), hash_uint16_t_u, compare_uint16_t)));
    REQUIRE(hs.size == 0);
    REQUIRE(hs.available == 16);

    REQUIRE(hs.header->capacity == 16);
    REQUIRE(hash_set_capacity(&hs) == 16);
    REQUIRE(hs.header->key_size == sizeof(K));
    REQUIRE(hs.header->key_align == alignof(K));

    hash_set_deinit(&hs);

    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 1, sizeof(K), alignof(K), hash_uint16_t_u, compare_uint16_t)));
    REQUIRE(hash_set_capacity(&hs) == HASH_SET_MINIMUM_CAPACITY);
    hash_set_deinit(&hs);
}

HASH_INTEGER_FN(uint32_t)
COMPARE_INTEGER_FN(uint32_t)

TEST_CASE("Basic set usage") {
    using K = uint32_t;

    HashSet hs;
    REQUIRE(STATUS_OK(
        hash_set_init(&hs, 10, sizeof(K), alignof(K), hash_uint32_t_u, compare_uint32_t)));
    const size_t count = 5;

    K load_total = 0;
    for (K i = 0; i < count; i++) {
        REQUIRE(STATUS_OK(hash_set_put(&hs, &i)));
        load_total += i;
    }
    REQUIRE(hash_set_count(&hs) == 5);

    K               internal_total = 0;
    HashSetIterator it             = hash_set_iterator_init(&hs);

    SetEntry e;
    while (hash_set_iterator_has_next(&it, &e)) {
        internal_total += *static_cast<K*>(e.key_ptr);
    }
    REQUIRE(load_total == internal_total);

    hash_set_deinit(&hs);
}

COMPARE_INTEGER_FN(int32_t);

TEST_CASE("Ensure total set capacity") {
    using K = int32_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint32_t_s, compare_int32_t)));

    REQUIRE(STATUS_OK(hash_set_ensure_total_capacity(&hs, 20)));
    const size_t initial_capacity = hash_set_capacity(&hs);
    REQUIRE(initial_capacity >= 20);

    for (K i = 0; i < 20; i++) {
        SetGetOrPutResult result;
        REQUIRE(STATUS_OK(hash_set_get_or_put(&hs, &i, &result)));
        REQUIRE_FALSE(result.found_existing);
    }
    REQUIRE(initial_capacity == hash_set_capacity(&hs));

    hash_set_deinit(&hs);
}

HASH_INTEGER_FN(uint64_t)
COMPARE_INTEGER_FN(uint64_t)

TEST_CASE("Ensure unused set capacity") {
    using K = int64_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint64_t_u, compare_uint64_t)));

    REQUIRE(STATUS_OK(hash_set_ensure_unused_capacity(&hs, 32)));
    const size_t capacity = hash_set_capacity(&hs);
    REQUIRE(STATUS_OK(hash_set_ensure_unused_capacity(&hs, 32)));
    REQUIRE(capacity == hash_set_capacity(&hs));

    hash_set_deinit(&hs);
}

TEST_CASE("Ensure unused set capacity with tombstones") {
    using K = int32_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint32_t_s, compare_int32_t)));

    for (K i = 0; i < 100; i++) {
        REQUIRE(STATUS_OK(hash_set_ensure_unused_capacity(&hs, 1)));
        hash_set_put_assume_capacity(&hs, &i);
        REQUIRE(STATUS_OK(hash_set_remove(&hs, &i)));
    }

    hash_set_deinit(&hs);
}

TEST_CASE("Clear retaining set capacity") {
    using K = Slice;

    HashSet hs;
    REQUIRE(STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_slice, compare_int32_t)));
    hash_set_clear_retaining_capacity(&hs);

    const char* str1 = "Hello";
    K           key1 = slice_from_str_z(str1);
    REQUIRE(STATUS_OK(hash_set_put(&hs, &key1)));
    REQUIRE(hash_set_count(&hs) == 1);

    hash_set_clear_retaining_capacity(&hs);
    hash_set_put_assume_capacity(&hs, &key1);
    REQUIRE(hash_set_count(&hs) == 1);

    const size_t capacity = hash_set_capacity(&hs);
    REQUIRE(capacity > 0);

    hash_set_clear_retaining_capacity(&hs);
    hash_set_clear_retaining_capacity(&hs);

    REQUIRE(hash_set_count(&hs) == 0);
    REQUIRE(hash_set_capacity(&hs) == capacity);
    REQUIRE_FALSE(hash_set_contains(&hs, &key1));

    hash_set_deinit(&hs);
}

TEST_CASE("Grow set") {
    using K = uint32_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint32_t_u, compare_uint32_t)));

    const size_t grow_to = 12456;

    for (size_t i = 0; i < grow_to; i++) {
        REQUIRE(STATUS_OK(hash_set_put(&hs, &i)));
    }
    REQUIRE(hash_set_count(&hs) == grow_to);

    HashSetIterator it = hash_set_iterator_init(&hs);
    SetEntry        e;
    size_t          total = 0;
    while (hash_set_iterator_has_next(&it, &e)) {
        total += 1;
    }
    REQUIRE(total == grow_to);

    hash_set_deinit(&hs);
}

TEST_CASE("Rehash set") {
    using K = uint32_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint32_t_u, compare_uint32_t)));

    // Add some elements and remove every third to simulate a fragmented map
    const size_t total_count = 6UZ * 1637UZ;
    for (size_t i = 0; i < total_count; i++) {
        REQUIRE(STATUS_OK(hash_set_put(&hs, &i)));
        if (i % 3 == 0) { REQUIRE(STATUS_OK(hash_set_remove(&hs, &i))); }
    }

    // Rehash and ensure data was not lost along the way
    hash_set_rehash(&hs);
    REQUIRE(hash_set_count(&hs) == total_count * 2 / 3);
    for (size_t i = 0; i < total_count; i++) {
        size_t out_index;
        if (i % 3 == 0) {
            REQUIRE(hash_set_get_index(&hs, &i, &out_index) == Status::ELEMENT_MISSING);
        } else {
            REQUIRE(STATUS_OK(hash_set_get_index(&hs, &i, &out_index)));
        }
    }

    hash_set_deinit(&hs);
}

TEST_CASE("Remove set") {
    using K = uint32_t;

    HashSet hs;
    REQUIRE(
        STATUS_OK(hash_set_init(&hs, 8, sizeof(K), alignof(K), hash_uint32_t_u, compare_uint32_t)));

    for (K i = 0; i < 16; i++) {
        REQUIRE(STATUS_OK(hash_set_put(&hs, &i)));
    }

    for (K i = 0; i < 16; i++) {
        if (i % 3 == 0) { REQUIRE(STATUS_OK(hash_set_remove(&hs, &i))); }
    }
    REQUIRE(hash_set_count(&hs) == 10);

    HashSetIterator it = hash_set_iterator_init(&hs);
    SetEntry        e;
    while (hash_set_iterator_has_next(&it, &e)) {
        const K k = *static_cast<K*>(e.key_ptr);
        REQUIRE(k % 3 != 0);
    }

    for (K i = 0; i < 16; i++) {
        if (i % 3 == 0) {
            REQUIRE_FALSE(hash_set_contains(&hs, &i));
        } else {
            size_t out_index;
            REQUIRE(STATUS_OK(hash_set_get_index(&hs, &i, &out_index)));
        }
    }

    hash_set_deinit(&hs);
}
