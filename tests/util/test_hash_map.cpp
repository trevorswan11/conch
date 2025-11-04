#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

#include <vector>

extern "C" {
#include "util/hash_map.h"
}

TEST_CASE("HashMap init") {
    using Key   = uint16_t;
    using Value = uint32_t;

    auto hash    = [](const void* a) { return (Hash)(*(Key*)a); };
    auto compare = [](const void* a, const void* b) { return (int)(*(Key*)a - *(Key*)b); };

    HashMap hm;
    hash_map_init(&hm, 10, sizeof(Key), alignof(Key), sizeof(Value), alignof(Value), hash, compare);
    REQUIRE(hm.size == 0);
    REQUIRE(hm.available == 10);

    REQUIRE(hm.header->capacity == 10);
    REQUIRE(hm.header->key_size == sizeof(Key));
    REQUIRE(hm.header->key_align == alignof(Key));
    REQUIRE(hm.header->value_size == sizeof(Value));
    REQUIRE(hm.header->value_align == alignof(Value));

    for (size_t i = 0; i < hm.header->capacity; i++) {
        REQUIRE(metadata_equal(hm.metadata[i], METADATA_SLOT_OPEN));
    }

    hash_map_deinit(&hm);
}
