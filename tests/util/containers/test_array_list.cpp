#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

#include <vector>

extern "C" {
#include "util/containers/array_list.h"
}

TEST_CASE("Init and resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)));
    REQUIRE(array_list_resize(&a, 12));
    REQUIRE(array_list_resize(&a, 8));
    array_list_deinit(&a);
}

TEST_CASE("Null safety") {
    ArrayList* a = NULL;
    REQUIRE_FALSE(array_list_resize(a, 10));

    int val = 42;
    REQUIRE_FALSE(array_list_push(a, &val));
    REQUIRE_FALSE(array_list_get(a, 0));

    int out;
    REQUIRE_FALSE(array_list_pop(a, &out));
    array_list_deinit(a);
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
        uint8_t item = *(uint8_t*)array_list_get(&a, i);
        REQUIRE(item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val));
    REQUIRE_FALSE(array_list_set(&a, 100, &manual_val));
    uint8_t item = *(uint8_t*)array_list_get(&a, 1);
    REQUIRE(item == manual_val);

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
        uint8_t item = *(uint8_t*)array_list_get(&a, i);
        REQUIRE(item == expecteds[i]);
    }

    // Push and check the remaining elements
    for (size_t i = chunk; i < expecteds.size(); i++) {
        array_list_push(&a, &expecteds[i]);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t item = *(uint8_t*)array_list_get(&a, i);
        REQUIRE(item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val));
    REQUIRE_FALSE(array_list_set(&a, 100, &manual_val));
    uint8_t item = *(uint8_t*)array_list_get(&a, 1);
    REQUIRE(item == manual_val);

    array_list_deinit(&a);
}

TEST_CASE("Remove ops") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint32_t)));

    uint32_t out;
    REQUIRE_FALSE(array_list_pop(&a, &out));

    uint32_t in = 10;
    array_list_push(&a, &in);
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

    uint32_t target  = 10;
    auto     compare = [](const void* a, const void* b) {
        return (int)(*(uint32_t*)a - *(uint32_t*)b);
    };
    REQUIRE(array_list_remove_item(&a, &target, compare));

    std::vector<uint32_t> remaining = {7, 2, 6, 22, 3};
    REQUIRE(a.length == remaining.size());
    for (size_t i = 0; i < remaining.size(); i++) {
        uint32_t item = *(uint32_t*)array_list_get(&a, i);
        REQUIRE(item == remaining[i]);
    }

    array_list_deinit(&a);
}

TEST_CASE("Find") {
    ArrayList a;
    array_list_init(&a, 4, sizeof(int));

    std::vector<int> values = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        array_list_push(&a, &values[i]);
    }

    auto   compare = [](const void* a, const void* b) { return *(int*)a - *(int*)b; };
    int    target  = 30;
    size_t idx;
    bool   found = array_list_find(&a, &idx, &target, compare);

    REQUIRE(found);
    REQUIRE(idx == 2);

    array_list_deinit(&a);
}
