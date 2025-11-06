#include "catch_amalgamated.hpp"

#include <stdint.h>

extern "C" {
#include "util/hash.h"
#include "util/mem.h"
}

TEST_CASE("compare_string_z") {
    const char* s1 = "hello";
    const char* s2 = "hello";
    const char* s3 = "world";

    REQUIRE(compare_string_z(s1, s2) != 0);
    REQUIRE(compare_string_z(s1, s3) == 0);
}

TEST_CASE("compare_slices") {
    Slice s1 = slice_from("foo", 3);
    Slice s2 = slice_from("foo", 3);
    Slice s3 = slice_from("bar", 3);

    REQUIRE(compare_slices(&s1, &s2) != 0);
    REQUIRE(compare_slices(&s1, &s3) == 0);
}

TEST_CASE("hash_slice") {
    Slice s1 = slice_from("hashme", 6);
    Slice s2 = slice_from("hashme", 6);
    Slice s3 = slice_from("different", 9);

    uint64_t h1 = hash_slice(&s1);
    uint64_t h2 = hash_slice(&s2);
    uint64_t h3 = hash_slice(&s3);

    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
}

TEST_CASE("hash_string_z") {
    const char* str1 = "Catch2";
    const char* str2 = "Catch2";
    const char* str3 = "catch2";

    uint64_t h1 = hash_string_z(str1);
    uint64_t h2 = hash_string_z(str2);
    uint64_t h3 = hash_string_z(str3);

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
    uint8_t a = 42, b = 42, c = 100;
    REQUIRE(hash_uint8_t_u(&a) == hash_uint8_t_u(&b));
    REQUIRE(hash_uint8_t_u(&a) != hash_uint8_t_u(&c));

    REQUIRE(hash_uint8_t_s(&a) == hash_uint8_t_u(&a));
}

TEST_CASE("hash_uint16_t") {
    uint16_t a = 1234, b = 1234, c = 5678;
    REQUIRE(hash_uint16_t_u(&a) == hash_uint16_t_u(&b));
    REQUIRE(hash_uint16_t_u(&a) != hash_uint16_t_u(&c));

    REQUIRE(hash_uint16_t_s(&a) == hash_uint16_t_u(&a));
}

TEST_CASE("hash_uint32_t") {
    uint32_t a = 0xDEADBEEF, b = 0xDEADBEEF, c = 0xFEEDBEEF;
    REQUIRE(hash_uint32_t_u(&a) == hash_uint32_t_u(&b));
    REQUIRE(hash_uint32_t_u(&a) != hash_uint32_t_u(&c));

    REQUIRE(hash_uint32_t_s(&a) == hash_uint32_t_u(&a));
}

TEST_CASE("hash_uint64_t") {
    uint64_t a = 0xDEADBEEFCAFEBABEULL, b = 0xDEADBEEFCAFEBABEULL, c = 0xFEEDFACE12345678ULL;
    REQUIRE(hash_uint64_t_u(&a) == hash_uint64_t_u(&b));
    REQUIRE(hash_uint64_t_u(&a) != hash_uint64_t_u(&c));

    REQUIRE(hash_uint64_t_s(&a) == hash_uint64_t_u(&a));
}

TEST_CASE("compare_uint8_t") {
    uint8_t a = 10, b = 20;
    REQUIRE(compare_uint8_t(&a, &b) == -1);
    REQUIRE(compare_uint8_t(&b, &a) == 1);
    REQUIRE(compare_uint8_t(&a, &a) == 0);
}

TEST_CASE("compare_uint16_t") {
    uint16_t a = 1000, b = 5000;
    REQUIRE(compare_uint16_t(&a, &b) == -1);
    REQUIRE(compare_uint16_t(&b, &a) == 1);
    REQUIRE(compare_uint16_t(&a, &a) == 0);
}

TEST_CASE("compare_uint32_t") {
    uint32_t a = 12345678, b = 87654321;
    REQUIRE(compare_uint32_t(&a, &b) == -1);
    REQUIRE(compare_uint32_t(&b, &a) == 1);
    REQUIRE(compare_uint32_t(&a, &a) == 0);
}

TEST_CASE("compare_uint64_t") {
    uint64_t a = 0x123456789ABCDEF0ULL, b = 0xFEDCBA9876543210ULL;
    REQUIRE(compare_uint64_t(&a, &b) == -1);
    REQUIRE(compare_uint64_t(&b, &a) == 1);
    REQUIRE(compare_uint64_t(&a, &a) == 0);
}
