#include "catch_amalgamated.hpp"
#include "fixtures.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

extern "C" {
#include "util/containers/array_list.h"
#include "util/hash.h"
}

TEST_CASE("Init and resize") {
    ArrayList a;

    REQUIRE(array_list_init(&a, 10, 0) == Status::ZERO_ITEM_SIZE);
    REQUIRE(STATUS_OK(array_list_init(&a, 10, sizeof(uint8_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    REQUIRE(array_list_capacity(&a) == 10);
    REQUIRE(STATUS_OK(array_list_resize(&a, 12)));
    REQUIRE(array_list_capacity(&a) == 12);
    REQUIRE(STATUS_OK(array_list_resize(&a, 8)));
    REQUIRE(array_list_capacity(&a) == 8);

    REQUIRE(STATUS_OK(array_list_ensure_total_capacity(&a, 7)));
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(STATUS_OK(array_list_ensure_total_capacity(&a, 8)));
    REQUIRE(array_list_capacity(&a) == 8);
    REQUIRE(STATUS_OK(array_list_ensure_total_capacity(&a, 20)));
    REQUIRE(array_list_capacity(&a) == 20);

    ArrayList b;
    REQUIRE(STATUS_OK(array_list_init(&b, 10, sizeof(uint8_t))));
    REQUIRE(STATUS_OK(array_list_resize(&b, 0)));
}

TEST_CASE("Push ops w/o resize") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 10, sizeof(uint8_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    // Push a handful of elements and check
    const auto expecteds = std::to_array<std::uint8_t>({4, 5, 7, 2, 6, 10, 22, 3});
    for (const auto& expected : expecteds) {
        array_list_push_assume_capacity(&a, &expected);
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out_item)));
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(STATUS_OK(array_list_set(&a, 1, &manual_val)));
    REQUIRE(array_list_set(&a, 100, &manual_val) == Status::INDEX_OUT_OF_BOUNDS);
    uint8_t out_item;
    REQUIRE(STATUS_OK(array_list_get(&a, 1, &out_item)));
    REQUIRE(out_item == manual_val);
}

TEST_CASE("Push ops w/ resize") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 10, sizeof(uint8_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};
    const auto               expecteds =
        std::to_array<std::uint8_t>({4, 5, 7, 2, 6, 10, 22, 3, 100, 2, 3, 7, 1, 2, 50, 2});

    // Push and check the first few elements
    const size_t chunk = 8;
    for (size_t i = 0; i < chunk; i++) {
        REQUIRE(STATUS_OK(array_list_push(&a, &expecteds[i])));
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out_item)));
        REQUIRE(out_item == expecteds[i]);
    }

    // Push and check the remaining elements
    for (size_t i = chunk; i < expecteds.size(); i++) {
        REQUIRE(STATUS_OK(array_list_push(&a, &expecteds[i])));
    }

    for (size_t i = 0; i < a.length; i++) {
        uint8_t out_item;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out_item)));
        REQUIRE(out_item == expecteds[i]);
    }

    // Set manually
    uint8_t manual_val = 42;
    REQUIRE(STATUS_OK(array_list_set(&a, 1, &manual_val)));
    REQUIRE(array_list_set(&a, 100, &manual_val) == Status::INDEX_OUT_OF_BOUNDS);
    uint8_t out_item;
    REQUIRE(STATUS_OK(array_list_get(&a, 1, &out_item)));
    REQUIRE(out_item == manual_val);
    uint8_t* mut_item;
    REQUIRE(STATUS_OK(array_list_get_ptr(&a, 1, (void**)&mut_item)));
    REQUIRE(mut_item);
    REQUIRE(*mut_item == manual_val);

    const uint8_t new_val = manual_val + 1;
    *mut_item             = new_val;
    REQUIRE(array_list_get_ptr(&a, 100, reinterpret_cast<void**>(&mut_item)) ==
            Status::INDEX_OUT_OF_BOUNDS);
    REQUIRE(STATUS_OK(array_list_get(&a, 1, &out_item)));
    REQUIRE(out_item == new_val);

    REQUIRE(STATUS_OK(array_list_resize(&a, 2)));
    REQUIRE(array_list_capacity(&a) == 2);
    REQUIRE(array_list_length(&a) == 2);
}

COMPARE_INTEGER_FN(uint32_t); // NOLINT

TEST_CASE("Remove ops") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 10, sizeof(uint32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    uint32_t out;
    REQUIRE(array_list_pop(&a, &out) == Status::EMPTY);

    uint32_t in = 10;
    REQUIRE(STATUS_OK(array_list_push(&a, &in)));
    REQUIRE(STATUS_OK(array_list_pop(&a, &out)));
    REQUIRE(out == in);

    // Push many items and arbitrarily remove
    std::vector<uint32_t> expecteds = {4, 5, 7, 2, 6, 10, 22, 3};
    std::vector<uint32_t> original  = expecteds;
    for (unsigned int& expected : expecteds) {
        REQUIRE(STATUS_OK(array_list_push(&a, &expected)));
    }

    REQUIRE(STATUS_OK(array_list_remove(&a, 0, nullptr)));
    REQUIRE(STATUS_OK(array_list_remove(&a, 0, &out)));
    REQUIRE(out == 5);
    REQUIRE(array_list_remove(&a, 100, &out) == Status::INDEX_OUT_OF_BOUNDS);

    uint32_t target = 10;
    REQUIRE(STATUS_OK(array_list_remove_item(&a, &target, compare_uint32_t)));

    const auto remaining = std::to_array<uint32_t>({7, 2, 6, 22, 3});
    REQUIRE(a.length == remaining.size());
    for (size_t i = 0; i < remaining.size(); i++) {
        uint32_t out_item;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out_item)));
        REQUIRE(out_item == remaining[i]);
    }

    // Make sure operations on the array did not impact the original vector
    for (size_t i = 0; i < original.size(); i++) {
        REQUIRE(expecteds[i] == original[i]);
    }
}

TEST_CASE("Stable insertion") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 4, sizeof(uint32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    const auto initial = std::to_array<uint32_t>({10, 20, 30, 40});
    for (auto v : initial) {
        REQUIRE(STATUS_OK(array_list_push(&a, &v)));
    }

    uint32_t new_val = 25;
    REQUIRE(STATUS_OK(array_list_insert_stable(&a, 2, &new_val)));
    const auto expected_stable = std::to_array<uint32_t>({10, 20, 25, 30, 40});
    REQUIRE(a.length == expected_stable.size());
    for (size_t i = 0; i < expected_stable.size(); i++) {
        uint32_t out;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out)));
        REQUIRE(out == expected_stable[i]);
    }

    uint32_t begin_val = 5;
    REQUIRE(STATUS_OK(array_list_insert_stable(&a, 0, &begin_val)));
    const auto expected_begin = std::to_array<uint32_t>({5, 10, 20, 25, 30, 40});
    for (size_t i = 0; i < expected_begin.size(); i++) {
        uint32_t out;
        REQUIRE(STATUS_OK(array_list_get(&a, i, &out)));
        REQUIRE(out == expected_begin[i]);
    }

    REQUIRE(STATUS_OK(array_list_ensure_total_capacity(&a, 10)));

    uint32_t end_val = 99;
    array_list_insert_stable_assume_capacity(&a, a.length, &end_val);
    REQUIRE(a.length == expected_begin.size() + 1);
    uint32_t last;
    REQUIRE(STATUS_OK(array_list_get(&a, a.length - 1, &last)));
    REQUIRE(last == 99);
    REQUIRE(array_list_is_sorted(&a, compare_uint32_t));
}

TEST_CASE("Unstable insertion") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 4, sizeof(uint32_t))));
    const Fixture<ArrayList> alfa{a, array_list_deinit};
    ArrayList                b;
    REQUIRE(STATUS_OK(array_list_init(&b, 4, sizeof(uint32_t))));
    const Fixture<ArrayList> alfb{b, array_list_deinit};

    const std::vector<uint32_t> initial = {10, 20, 30, 40};
    for (auto v : initial) {
        REQUIRE(STATUS_OK(array_list_push(&a, &v)));
        REQUIRE(STATUS_OK(array_list_push(&b, &v)));
    }

    uint32_t new_val_u = 25;
    REQUIRE(STATUS_OK(array_list_insert_unstable(&b, 2, &new_val_u)));
    REQUIRE(array_list_length(&b) == initial.size() + 1);
    uint32_t inserted;
    REQUIRE(STATUS_OK(array_list_get(&b, 2, &inserted)));
    REQUIRE(inserted == 25);

    std::vector<uint32_t> observed;
    for (size_t i = 0; i < b.length; i++) {
        uint32_t v;
        REQUIRE(STATUS_OK(array_list_get(&b, i, &v)));
        observed.push_back(v);
    }
    std::vector<uint32_t> expected_contents = initial;
    expected_contents.push_back(new_val_u);
    std::ranges::sort(observed);
    std::ranges::sort(expected_contents);
    REQUIRE(observed == expected_contents);

    REQUIRE(STATUS_OK(array_list_ensure_total_capacity(&b, 10)));

    uint32_t front = 1;
    array_list_insert_unstable_assume_capacity(&b, 0, &front);
    uint32_t back = 99;
    array_list_insert_unstable_assume_capacity(&b, b.length, &back);
    REQUIRE(b.length == initial.size() + 3);

    uint32_t first;
    uint32_t last_u;
    REQUIRE(STATUS_OK(array_list_get(&b, 0, &first)));
    REQUIRE(STATUS_OK(array_list_get(&b, b.length - 1, &last_u)));
    REQUIRE(first == front);
    REQUIRE(last_u == back);
}

COMPARE_INTEGER_FN(int32_t); // NOLINT

TEST_CASE("Malformed find") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 4, sizeof(int32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    size_t  maybe_idx;
    int32_t maybe_item = 0;
    REQUIRE(array_list_find(&a, nullptr, &maybe_item, compare_int32_t) == Status::NULL_PARAMETER);
    REQUIRE(array_list_find(&a, &maybe_idx, nullptr, compare_int32_t) == Status::NULL_PARAMETER);
    REQUIRE(array_list_find(&a, &maybe_idx, &maybe_item, nullptr) == Status::NULL_PARAMETER);
}

TEST_CASE("Find") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 4, sizeof(int32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    const auto values = std::to_array<int32_t>({10, 20, 30, 40});
    for (const int& value : values) {
        REQUIRE(STATUS_OK(array_list_push(&a, &value)));
    }

    int32_t present_target = 30;
    size_t  idx;
    REQUIRE(STATUS_OK(array_list_find(&a, &idx, &present_target, compare_int32_t)));
    REQUIRE(idx == 2);

    int32_t missing_target = 31;
    REQUIRE(array_list_find(&a, &idx, &missing_target, compare_int32_t) == Status::ELEMENT_MISSING);
    REQUIRE(idx == 2);
}

TEST_CASE("Sorting and binary search") {
    ArrayList    a;
    const size_t total_size = 1000;
    REQUIRE(STATUS_OK(array_list_init(&a, 1.5 * total_size, sizeof(uint32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};

    // Pack a vector and array list with random numbers for sorting
    std::vector<uint32_t> random;
    random.reserve(total_size);
    for (size_t i = 0; i < total_size; ++i) {
        random.push_back(std::rand() % 10 * total_size);
    }

    for (unsigned int& i : random) {
        REQUIRE(STATUS_OK(array_list_push(&a, &i)));
    }

    // Sort both containers and compare
    std::ranges::sort(random);
    REQUIRE_FALSE(array_list_is_sorted(&a, compare_uint32_t));
    array_list_sort(&a, compare_uint32_t);
    REQUIRE(array_list_is_sorted(&a, compare_uint32_t));

    size_t   search_index;
    uint32_t elem;
    uint32_t search_elem;
    for (size_t i = 0; i < random.size(); i++) {
        REQUIRE(STATUS_OK(array_list_get(&a, i, &elem)));
        REQUIRE(random[i] == elem);

        // We can't compare with i here because rand is 'with-replacement'
        REQUIRE(array_list_bsearch(&a, &search_index, &elem, compare_uint32_t));
        REQUIRE(STATUS_OK(array_list_get(&a, search_index, &search_elem)));
        REQUIRE(elem == search_elem);
    }
}

TEST_CASE("ArrayList iterator patterns") {
    ArrayList a;
    REQUIRE(STATUS_OK(array_list_init(&a, 4, sizeof(uint32_t))));
    const Fixture<ArrayList> alf{a, array_list_deinit};
    const auto               values = std::to_array<uint32_t>({10, 20, 30, 40});

    SECTION("Mutable iterator") {
        for (auto v : values) {
            REQUIRE(STATUS_OK(array_list_push(&a, &v)));
        }
        REQUIRE(a.length == values.size());

        auto      it = array_list_iterator_init(&a);
        size_t    i  = 0;
        uint32_t* out;
        uint32_t  check;
        while (array_list_iterator_has_next(&it, reinterpret_cast<void**>(&out))) {
            REQUIRE(STATUS_OK(array_list_get(&a, i, &check)));
            REQUIRE(*out == values[i]);
            REQUIRE(*out == check);

            *out *= 2;
            i += 1;
        }
    }

    SECTION("Const iterator") {
        auto     it = array_list_const_iterator_init(&a);
        size_t   i  = 0;
        uint32_t out;
        uint32_t check;
        while (array_list_const_iterator_has_next(&it, &out)) {
            REQUIRE(STATUS_OK(array_list_get(&a, i, &check)));
            REQUIRE(out == values[i] * 2);
            REQUIRE(out == check);
            i += 1;
        }
    }
}

TEST_CASE("Copying array lists") {
    using V = uint32_t;
    ArrayList source;
    REQUIRE(STATUS_OK(array_list_init(&source, 200, sizeof(V))));
    const Fixture<ArrayList> source_fix(source, array_list_deinit);

    std::array<V, 100> values;
    std::iota(values.begin(), values.end(), 0);
    for (auto v : values) {
        REQUIRE(STATUS_OK(array_list_push(&source, &v)));
    }

    const auto test_array = [&](const ArrayList& array) -> void {
        V out;
        for (size_t i = 0; i < values.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&array, i, &out)));
            REQUIRE(values[i] == out);
        }
    };

    test_array(source);

    ArrayList dest;
    REQUIRE(STATUS_OK(array_list_copy_from(&dest, &source)));
    const Fixture<ArrayList> dest_fix(dest, array_list_deinit);

    REQUIRE(source.item_size == dest.item_size);
    REQUIRE(dest.length == std::size(values));
    REQUIRE(dest.capacity == std::size(values));

    test_array(dest);
}
