#include "catch_amalgamated.hpp"

#include <cstdint>

extern "C" {
#include "util/hash.h"
#include "util/memory.h"
}

TEST_CASE("Null terminated string comparison") {
    const char* s1 = "hello";
    const char* s2 = "hello";
    const char* s3 = "world";

    REQUIRE(compare_string_z(s1, s2) == 0);
    REQUIRE(compare_string_z(s1, s3) != 0);
}

TEST_CASE("Slice comparison") {
    Slice s1 = slice_from_str_s("foo", 3);
    Slice s2 = slice_from_str_s("foo", 3);
    Slice s3 = slice_from_str_s("bar", 3);

    REQUIRE(compare_slices(&s1, &s2) == 0);
    REQUIRE(compare_slices(&s1, &s3) != 0);
}

TEST_CASE("Mutable slice comparison") {
    MutSlice m1 = {.ptr = "foo", .length = 3};
    MutSlice m2 = {.ptr = "foo", .length = 3};
    MutSlice m3 = {.ptr = "bar", .length = 3};

    REQUIRE(compare_mut_slices(&m1, &m2) == 0);
    REQUIRE(compare_mut_slices(&m1, &m3) != 0);

    MutSlice m4 = {.ptr = "foobar", .length = 6};
    REQUIRE(compare_mut_slices(&m1, &m4) != 0);
}

TEST_CASE("Null terminating string hashing") {
    const char* str1 = "Catch2";
    const char* str2 = "Catch2";
    const char* str3 = "catch2";

    const uint64_t h1 = hash_string_z(str1);
    const uint64_t h2 = hash_string_z(str2);
    const uint64_t h3 = hash_string_z(str3);

    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
}

TEST_CASE("Slice hashing") {
    const Slice s1 = slice_from_str_s("hashme", 6);
    const Slice s2 = slice_from_str_s("hashme", 6);
    const Slice s3 = slice_from_str_s("different", 9);

    const uint64_t h1 = hash_slice(&s1);
    const uint64_t h2 = hash_slice(&s2);
    const uint64_t h3 = hash_slice(&s3);

    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
}

TEST_CASE("Mutable slice hashing") {
    const MutSlice m1 = {.ptr = "hashme", .length = 6};
    const MutSlice m2 = {.ptr = "hashme", .length = 6};
    const MutSlice m3 = {.ptr = "different", .length = 9};

    const uint64_t h1 = hash_mut_slice(&m1);
    const uint64_t h2 = hash_mut_slice(&m2);
    const uint64_t h3 = hash_mut_slice(&m3);

    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
}

HASH_INTEGER_FN(uint8_t)
HASH_INTEGER_FN(uint16_t)
HASH_INTEGER_FN(uint32_t)
HASH_INTEGER_FN(uint64_t)

COMPARE_INTEGER_FN(uint8_t)
COMPARE_INTEGER_FN(uint16_t)
COMPARE_INTEGER_FN(uint32_t)
COMPARE_INTEGER_FN(uint64_t)

TEST_CASE("hash_uint8_t") {
    uint8_t a = 42;
    uint8_t b = 42;
    uint8_t c = 100;
    REQUIRE(hash_uint8_t_u(&a) == hash_uint8_t_u(&b));
    REQUIRE(hash_uint8_t_u(&a) != hash_uint8_t_u(&c));

    REQUIRE(hash_uint8_t_s(&a) == hash_uint8_t_u(&a));
}

TEST_CASE("hash_uint16_t") {
    uint16_t a = 1234;
    uint16_t b = 1234;
    uint16_t c = 5678;
    REQUIRE(hash_uint16_t_u(&a) == hash_uint16_t_u(&b));
    REQUIRE(hash_uint16_t_u(&a) != hash_uint16_t_u(&c));

    REQUIRE(hash_uint16_t_s(&a) == hash_uint16_t_u(&a));
}

TEST_CASE("hash_uint32_t") {
    uint32_t a = 0xDEADBEEF;
    uint32_t b = 0xDEADBEEF;
    uint32_t c = 0xFEEDBEEF;
    REQUIRE(hash_uint32_t_u(&a) == hash_uint32_t_u(&b));
    REQUIRE(hash_uint32_t_u(&a) != hash_uint32_t_u(&c));

    REQUIRE(hash_uint32_t_s(&a) == hash_uint32_t_u(&a));
}

TEST_CASE("hash_uint64_t") {
    uint64_t a = 0xDEADBEEFCAFEBABEULL;
    uint64_t b = 0xDEADBEEFCAFEBABEULL;
    uint64_t c = 0xFEEDFACE12345678ULL;
    REQUIRE(hash_uint64_t_u(&a) == hash_uint64_t_u(&b));
    REQUIRE(hash_uint64_t_u(&a) != hash_uint64_t_u(&c));

    REQUIRE(hash_uint64_t_s(&a) == hash_uint64_t_u(&a));
}

TEST_CASE("compare_uint8_t") {
    uint8_t a = 10;
    uint8_t b = 20;
    REQUIRE(compare_uint8_t(&a, &b) == -1);
    REQUIRE(compare_uint8_t(&b, &a) == 1);
    REQUIRE(compare_uint8_t(&a, &a) == 0);
}

TEST_CASE("compare_uint16_t") {
    uint16_t a = 1000;
    uint16_t b = 5000;
    REQUIRE(compare_uint16_t(&a, &b) == -1);
    REQUIRE(compare_uint16_t(&b, &a) == 1);
    REQUIRE(compare_uint16_t(&a, &a) == 0);
}

TEST_CASE("compare_uint32_t") {
    uint32_t a = 12345678;
    uint32_t b = 87654321;
    REQUIRE(compare_uint32_t(&a, &b) == -1);
    REQUIRE(compare_uint32_t(&b, &a) == 1);
    REQUIRE(compare_uint32_t(&a, &a) == 0);
}

TEST_CASE("compare_uint64_t") {
    uint64_t a = 0x123456789ABCDEF0ULL;
    uint64_t b = 0xFEDCBA9876543210ULL;
    REQUIRE(compare_uint64_t(&a, &b) == -1);
    REQUIRE(compare_uint64_t(&b, &a) == 1);
    REQUIRE(compare_uint64_t(&a, &a) == 0);
}
