#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

#include <vector>

extern "C" {
#include "util/containers/array_list.h"
#include "util/hash.h"
}

TEST_CASE("Init and resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)));
    REQUIRE(array_list_capacity(&a) == 10);
    REQUIRE(array_list_resize(&a, 12));
    REQUIRE(array_list_capacity(&a) == 12);
    REQUIRE(array_list_resize(&a, 8));
    REQUIRE(array_list_capacity(&a) == 8);

    REQUIRE(array_list_ensure_total_capacity(&a, 7));
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(array_list_ensure_total_capacity(&a, 8));
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(array_list_ensure_total_capacity(&a, 20));
    REQUIRE(array_list_capacity(&a) == 20);

    array_list_deinit(&a);

    ArrayList b;
    REQUIRE(array_list_init(&b, 10, sizeof(uint8_t)));
    REQUIRE(array_list_resize(&b, 0));
}

TEST_CASE("Push ops w/o resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)));

    // Push a handful of elements and check
    std::vector<uint8_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3};
    for (size_t i = 0; i < expecteds.size(); i++) {
        array_list_push(&a, &expecteds[i]);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item));
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val));
    REQUIRE_FALSE(array_list_set(&a, 100, &manual_val));
    uint8_t out_item;
    REQUIRE(array_list_get(&a, 1, &out_item));
    REQUIRE(out_item == manual_val);

    array_list_deinit(&a);
}

TEST_CASE("Push ops w/ resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)));
    std::vector<uint8_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3, 100, 2, 3, 7, 1, 2, 50, 2};

    // Push and check the first few elements
    const size_t chunk = 8;
    for (size_t i = 0; i < chunk; i++) {
        array_list_push(&a, &expecteds[i]);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item));
        REQUIRE(out_item == expecteds[i]);
    }

    // Push and check the remaining elements
    for (size_t i = chunk; i < expecteds.size(); i++) {
        array_list_push(&a, &expecteds[i]);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item));
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val));
    REQUIRE_FALSE(array_list_set(&a, 100, &manual_val));
    uint8_t out_item;
    REQUIRE(array_list_get(&a, 1, &out_item));
    REQUIRE(out_item == manual_val);
    uint8_t* mut_item = (uint8_t*)array_list_get_ptr(&a, 1);
    REQUIRE(mut_item);
    REQUIRE(*mut_item == manual_val);

    uint8_t new_val = manual_val + 1;
    *mut_item       = new_val;
    REQUIRE_FALSE(array_list_get_ptr(&a, 100));
    REQUIRE(array_list_get(&a, 1, &out_item));
    REQUIRE(out_item == new_val);

    REQUIRE(array_list_resize(&a, 2));
    REQUIRE(array_list_capacity(&a) == 2);
    REQUIRE(array_list_length(&a) == 2);

    array_list_deinit(&a);
}

COMPARE_INTEGER_FN(uint32_t);

TEST_CASE("Remove ops") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint32_t)));

    uint32_t out;
    REQUIRE_FALSE(array_list_pop(&a, &out));

    uint32_t in = 10;
    REQUIRE(array_list_push(&a, &in));
    REQUIRE(array_list_pop(&a, &out));
    REQUIRE(out == in);

    // Push many items and arbitrarily remove
    std::vector<uint32_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3};
    for (size_t i = 0; i < expecteds.size(); i++) {
        array_list_push(&a, &expecteds[i]);
    }

    REQUIRE(array_list_remove(&a, 0, NULL));
    REQUIRE(array_list_remove(&a, 0, &out));
    REQUIRE(out == 5);
    REQUIRE_FALSE(array_list_remove(&a, 100, &out));

    uint32_t target = 10;
    REQUIRE(array_list_remove_item(&a, &target, compare_uint32_t));

    std::vector<uint32_t> remaining = {7, 2, 6, 22, 3};
    REQUIRE(a.length == remaining.size());
    for (size_t i = 0; i < remaining.size(); i++) {
        uint32_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item));
        REQUIRE(out_item == remaining[i]);
    }

    array_list_deinit(&a);
}

COMPARE_INTEGER_FN(int32_t);

TEST_CASE("Malformed find") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 4, sizeof(int32_t)));

    size_t  maybe_idx;
    int32_t maybe_item;
    REQUIRE_FALSE(array_list_find(&a, NULL, &maybe_item, compare_int32_t));
    REQUIRE_FALSE(array_list_find(&a, &maybe_idx, NULL, compare_int32_t));
    REQUIRE_FALSE(array_list_find(&a, &maybe_idx, &maybe_item, NULL));

    array_list_deinit(&a);
}

TEST_CASE("Find") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 4, sizeof(int32_t)));

    std::vector<int32_t> values = {10, 20, 30, 40};
    for (int32_t i = 0; i < 4; i++) {
        array_list_push(&a, &values[i]);
    }

    int32_t present_target = 30;
    size_t  idx;
    REQUIRE(array_list_find(&a, &idx, &present_target, compare_int32_t));
    REQUIRE(idx == 2);

    int32_t missing_target = 31;
    REQUIRE_FALSE(array_list_find(&a, &idx, &missing_target, compare_int32_t));
    REQUIRE(idx == 2);

    array_list_deinit(&a);
}
