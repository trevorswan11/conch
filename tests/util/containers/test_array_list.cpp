#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <vector>

extern "C" {
#include "util/containers/array_list.h"
#include "util/error.h"
#include "util/hash.h"
}

TEST_CASE("Init and resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, 0) == AnyError::ZERO_ITEM_SIZE);
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 10);
    REQUIRE(array_list_resize(&a, 12));
    REQUIRE(array_list_capacity(&a) == 12);
    REQUIRE(array_list_resize(&a, 8) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 8);

    REQUIRE(array_list_ensure_total_capacity(&a, 7) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(array_list_ensure_total_capacity(&a, 8) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(array_list_ensure_total_capacity(&a, 20) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 20);

    array_list_deinit(&a);

    ArrayList b;
    REQUIRE(array_list_init(&b, 10, sizeof(uint8_t)));
    REQUIRE(array_list_resize(&b, 0) == AnyError::SUCCESS);
}

TEST_CASE("Push ops w/o resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)) == AnyError::SUCCESS);

    // Push a handful of elements and check
    std::vector<uint8_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3};
    for (size_t i = 0; i < expecteds.size(); i++) {
        array_list_push_assume_capacity(&a, &expecteds[i]);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item) == AnyError::SUCCESS);
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val) == AnyError::SUCCESS);
    REQUIRE(array_list_set(&a, 100, &manual_val) == AnyError::INDEX_OUT_OF_BOUNDS);
    uint8_t out_item;
    REQUIRE(array_list_get(&a, 1, &out_item) == AnyError::SUCCESS);
    REQUIRE(out_item == manual_val);

    array_list_deinit(&a);
}

TEST_CASE("Push ops w/ resize") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint8_t)) == AnyError::SUCCESS);
    std::vector<uint8_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3, 100, 2, 3, 7, 1, 2, 50, 2};

    // Push and check the first few elements
    const size_t chunk = 8;
    for (size_t i = 0; i < chunk; i++) {
        REQUIRE(array_list_push(&a, &expecteds[i]) == AnyError::SUCCESS);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item) == AnyError::SUCCESS);
        REQUIRE(out_item == expecteds[i]);
    }

    // Push and check the remaining elements
    for (size_t i = chunk; i < expecteds.size(); i++) {
        REQUIRE(array_list_push(&a, &expecteds[i]) == AnyError::SUCCESS);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item) == AnyError::SUCCESS);
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(array_list_set(&a, 1, &manual_val) == AnyError::SUCCESS);
    REQUIRE(array_list_set(&a, 100, &manual_val) == AnyError::INDEX_OUT_OF_BOUNDS);
    uint8_t out_item;
    REQUIRE(array_list_get(&a, 1, &out_item) == AnyError::SUCCESS);
    REQUIRE(out_item == manual_val);
    uint8_t* mut_item;
    REQUIRE(array_list_get_ptr(&a, 1, (void**)&mut_item) == AnyError::SUCCESS);
    REQUIRE(mut_item);
    REQUIRE(*mut_item == manual_val);

    uint8_t new_val = manual_val + 1;
    *mut_item       = new_val;
    REQUIRE(array_list_get_ptr(&a, 100, (void**)&mut_item) == AnyError::INDEX_OUT_OF_BOUNDS);
    REQUIRE(array_list_get(&a, 1, &out_item) == AnyError::SUCCESS);
    REQUIRE(out_item == new_val);

    REQUIRE(array_list_resize(&a, 2) == AnyError::SUCCESS);
    REQUIRE(array_list_capacity(&a) == 2);
    REQUIRE(array_list_length(&a) == 2);

    array_list_deinit(&a);
}

COMPARE_INTEGER_FN(uint32_t);

TEST_CASE("Remove ops") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 10, sizeof(uint32_t)) == AnyError::SUCCESS);

    uint32_t out;
    REQUIRE(array_list_pop(&a, &out) == AnyError::EMPTY);

    uint32_t in = 10;
    REQUIRE(array_list_push(&a, &in) == AnyError::SUCCESS);
    REQUIRE(array_list_pop(&a, &out) == AnyError::SUCCESS);
    REQUIRE(out == in);

    // Push many items and arbitrarily remove
    std::vector<uint32_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3};
    std::vector<uint32_t> original  = expecteds;
    for (size_t i = 0; i < expecteds.size(); i++) {
        array_list_push(&a, &expecteds[i]);
    }

    REQUIRE(array_list_remove(&a, 0, NULL) == AnyError::SUCCESS);
    REQUIRE(array_list_remove(&a, 0, &out) == AnyError::SUCCESS);
    REQUIRE(out == 5);
    REQUIRE(array_list_remove(&a, 100, &out) == AnyError::INDEX_OUT_OF_BOUNDS);

    uint32_t target = 10;
    REQUIRE(array_list_remove_item(&a, &target, compare_uint32_t) == AnyError::SUCCESS);

    std::vector<uint32_t> remaining = {7, 2, 6, 22, 3};
    REQUIRE(a.length == remaining.size());
    for (size_t i = 0; i < remaining.size(); i++) {
        uint32_t out_item;
        REQUIRE(array_list_get(&a, i, &out_item) == AnyError::SUCCESS);
        REQUIRE(out_item == remaining[i]);
    }

    // Make sure operations on the array did not impact the original vector
    for (size_t i = 0; i < original.size(); i++) {
        REQUIRE(expecteds[i] == original[i]);
    }

    array_list_deinit(&a);
}

TEST_CASE("Stable insertion") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 4, sizeof(uint32_t)) == AnyError::SUCCESS);

    std::vector<uint32_t> initial = {10, 20, 30, 40};
    for (auto v : initial) {
        REQUIRE(array_list_push(&a, &v) == AnyError::SUCCESS);
    }

    uint32_t new_val = 25;
    REQUIRE(array_list_insert_stable(&a, 2, &new_val) == AnyError::SUCCESS);
    std::vector<uint32_t> expected_stable = {10, 20, 25, 30, 40};
    REQUIRE(a.length == expected_stable.size());
    for (size_t i = 0; i < expected_stable.size(); i++) {
        uint32_t out;
        REQUIRE(array_list_get(&a, i, &out) == AnyError::SUCCESS);
        REQUIRE(out == expected_stable[i]);
    }

    uint32_t begin_val = 5;
    REQUIRE(array_list_insert_stable(&a, 0, &begin_val) == AnyError::SUCCESS);
    std::vector<uint32_t> expected_begin = {5, 10, 20, 25, 30, 40};
    for (size_t i = 0; i < expected_begin.size(); i++) {
        uint32_t out;
        REQUIRE(array_list_get(&a, i, &out) == AnyError::SUCCESS);
        REQUIRE(out == expected_begin[i]);
    }

    REQUIRE(array_list_ensure_total_capacity(&a, 10) == AnyError::SUCCESS);

    uint32_t end_val = 99;
    array_list_insert_stable_assume_capacity(&a, a.length, &end_val);
    REQUIRE(a.length == expected_begin.size() + 1);
    uint32_t last;
    REQUIRE(array_list_get(&a, a.length - 1, &last) == AnyError::SUCCESS);
    REQUIRE(last == 99);
    REQUIRE(array_list_is_sorted(&a, compare_uint32_t));

    array_list_deinit(&a);
}

TEST_CASE("Unstable insertion") {
    ArrayList a, b;
    REQUIRE(array_list_init(&a, 4, sizeof(uint32_t)) == AnyError::SUCCESS);
    REQUIRE(array_list_init(&b, 4, sizeof(uint32_t)) == AnyError::SUCCESS);

    std::vector<uint32_t> initial = {10, 20, 30, 40};
    for (auto v : initial) {
        REQUIRE(array_list_push(&a, &v) == AnyError::SUCCESS);
        REQUIRE(array_list_push(&b, &v) == AnyError::SUCCESS);
    }

    uint32_t new_val_u = 25;
    REQUIRE(array_list_insert_unstable(&b, 2, &new_val_u) == AnyError::SUCCESS);
    REQUIRE(array_list_length(&b) == initial.size() + 1);
    uint32_t inserted;
    REQUIRE(array_list_get(&b, 2, &inserted) == AnyError::SUCCESS);
    REQUIRE(inserted == 25);

    std::vector<uint32_t> observed;
    for (size_t i = 0; i < b.length; i++) {
        uint32_t v;
        REQUIRE(array_list_get(&b, i, &v) == AnyError::SUCCESS);
        observed.push_back(v);
    }
    std::vector<uint32_t> expected_contents = initial;
    expected_contents.push_back(new_val_u);
    std::sort(observed.begin(), observed.end());
    std::sort(expected_contents.begin(), expected_contents.end());
    REQUIRE(observed == expected_contents);

    REQUIRE(array_list_ensure_total_capacity(&b, 10));

    uint32_t front = 1;
    array_list_insert_unstable_assume_capacity(&b, 0, &front);
    uint32_t back = 99;
    array_list_insert_unstable_assume_capacity(&b, b.length, &back);
    REQUIRE(b.length == initial.size() + 3);

    uint32_t first, last_u;
    REQUIRE(array_list_get(&b, 0, &first) == AnyError::SUCCESS);
    REQUIRE(array_list_get(&b, b.length - 1, &last_u) == AnyError::SUCCESS);
    REQUIRE(first == front);
    REQUIRE(last_u == back);

    array_list_deinit(&a);
    array_list_deinit(&b);
}

COMPARE_INTEGER_FN(int32_t);

TEST_CASE("Malformed find") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 4, sizeof(int32_t)) == AnyError::SUCCESS);

    size_t  maybe_idx;
    int32_t maybe_item;
    REQUIRE(array_list_find(&a, NULL, &maybe_item, compare_int32_t) == AnyError::NULL_PARAMETER);
    REQUIRE(array_list_find(&a, &maybe_idx, NULL, compare_int32_t) == AnyError::NULL_PARAMETER);
    REQUIRE(array_list_find(&a, &maybe_idx, &maybe_item, NULL) == AnyError::NULL_PARAMETER);

    array_list_deinit(&a);
}

TEST_CASE("Find") {
    ArrayList a;
    REQUIRE(array_list_init(&a, 4, sizeof(int32_t)) == AnyError::SUCCESS);

    std::vector<int32_t> values = {10, 20, 30, 40};
    for (size_t i = 0; i < values.size(); i++) {
        REQUIRE(array_list_push(&a, &values[i]) == AnyError::SUCCESS);
    }

    int32_t present_target = 30;
    size_t  idx;
    REQUIRE(array_list_find(&a, &idx, &present_target, compare_int32_t) == AnyError::SUCCESS);
    REQUIRE(idx == 2);

    int32_t missing_target = 31;
    REQUIRE(array_list_find(&a, &idx, &missing_target, compare_int32_t) ==
            AnyError::ELEMENT_MISSING);
    REQUIRE(idx == 2);

    array_list_deinit(&a);
}

TEST_CASE("Sorting and binary search") {
    ArrayList    a;
    const size_t total_size = 1000;
    REQUIRE(array_list_init(&a, 1.5 * total_size, sizeof(uint32_t)) == AnyError::SUCCESS);

    // Pack a vector and array list with random numbers for sorting
    std::vector<uint32_t> random;
    for (size_t i = 0; i < total_size; ++i) {
        random.push_back(std::rand() % 10 * total_size);
    }

    for (size_t i = 0; i < random.size(); i++) {
        REQUIRE(array_list_push(&a, &random[i]) == AnyError::SUCCESS);
    }

    // Sort both containers and compare
    std::sort(random.begin(), random.end());
    REQUIRE_FALSE(array_list_is_sorted(&a, compare_uint32_t));
    array_list_sort(&a, compare_uint32_t);
    REQUIRE(array_list_is_sorted(&a, compare_uint32_t));

    size_t   search_index;
    uint32_t elem, search_elem;
    for (size_t i = 0; i < random.size(); i++) {
        REQUIRE(array_list_get(&a, i, &elem) == AnyError::SUCCESS);
        REQUIRE(random[i] == elem);

        // We can't compare with i here because rand is 'with-replacement'
        REQUIRE(array_list_bsearch(&a, &search_index, &elem, compare_uint32_t));
        REQUIRE(array_list_get(&a, search_index, &search_elem) == AnyError::SUCCESS);
        REQUIRE(elem == search_elem);
    }

    array_list_deinit(&a);
}
