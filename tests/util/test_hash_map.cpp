#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

#include <vector>

extern "C" {
#include "util/hash.h"
#include "util/hash_map.h"
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

HASH_INTEGER_FN(uint16_t);
COMPARE_INTEGER_FN(uint16_t);

TEST_CASE("Malformed map usage") {
    using K = uint16_t;
    using V = uint32_t;

    HashMap hm;

    // Null pointers should not allow initialization
    REQUIRE_FALSE(hash_map_init(
        NULL, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), NULL, compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, NULL));

    // Zero size/alignment is illegal
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, 0, alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, sizeof(K), 0, sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), 0, alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), 0, hash_uint16_t_u, compare_uint16_t));

    // Check capacity overflow safety guard
    REQUIRE_FALSE(hash_map_init(&hm,
                                SIZE_MAX / 2,
                                3,
                                alignof(K),
                                sizeof(V),
                                alignof(V),
                                hash_uint16_t_u,
                                compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(&hm,
                                SIZE_MAX / 2,
                                sizeof(K),
                                alignof(K),
                                3,
                                alignof(V),
                                hash_uint16_t_u,
                                compare_uint16_t));
    REQUIRE_FALSE(hash_map_init(&hm,
                                SIZE_MAX - 1,
                                sizeof(K),
                                alignof(K),
                                sizeof(V),
                                alignof(V),
                                hash_uint16_t_u,
                                compare_uint16_t));

    // Helpers that check self and buffer initialization
    REQUIRE_FALSE(hash_map_capacity(NULL));
    REQUIRE_FALSE(hash_map_count(NULL));
    REQUIRE_FALSE(hash_map_keys(NULL));
    REQUIRE_FALSE(hash_map_values(NULL));
    REQUIRE_FALSE(hash_map_ensure_total_capacity(NULL, 10));

    hm.buffer = NULL;
    REQUIRE_FALSE(hash_map_capacity(&hm));
    REQUIRE_FALSE(hash_map_count(&hm));
    REQUIRE_FALSE(hash_map_keys(&hm));
    REQUIRE_FALSE(hash_map_values(&hm));
    REQUIRE_FALSE(hash_map_ensure_total_capacity(&hm, 10));
}

TEST_CASE("HashMap init") {
    using K = uint16_t;
    using V = uint32_t;

    HashMap hm;
    REQUIRE(hash_map_init(
        &hm, 10, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE(hm.size == 0);
    REQUIRE(hm.available == 10);

    REQUIRE(hm.header->capacity == 10);
    REQUIRE(hash_map_capacity(&hm) == 10);
    REQUIRE(hm.header->key_size == sizeof(K));
    REQUIRE(hm.header->key_align == alignof(K));
    REQUIRE(hm.header->value_size == sizeof(V));
    REQUIRE(hm.header->value_align == alignof(V));

    for (size_t i = 0; i < hm.header->capacity; i++) {
        REQUIRE(metadata_equal(hm.metadata[i], METADATA_SLOT_OPEN));
    }

    hash_map_deinit(&hm);

    REQUIRE(hash_map_init(
        &hm, 1, sizeof(K), alignof(K), sizeof(V), alignof(V), hash_uint16_t_u, compare_uint16_t));
    REQUIRE(hash_map_capacity(&hm) == HM_MINIMUM_CAPACITY);
    hash_map_deinit(&hm);
}
