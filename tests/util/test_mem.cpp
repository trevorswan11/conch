#include "catch_amalgamated.hpp"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "util/math.h"
#include "util/mem.h"
}

TEST_CASE("Word size determined correctly") {
#ifdef WORD_SIZE_64
    REQUIRE(sizeof(size_t) == 8);
#elif defined(WORD_SIZE_32)
    REQUIRE(sizeof(size_t) == 4);
#endif
}

TEST_CASE("Slice creation and equality") {
    const char* text = "hello";
    Slice       s1   = slice_from_str_s(text, 5);
    Slice       s1_z = slice_from_str_z(text);
    Slice       s2   = slice_from_str_s("hello", 5);
    Slice       s3   = slice_from_str_s("hellx", 5);
    Slice       s4   = slice_from_str_s("hello world", 5);

    REQUIRE(s1.length == 5);
    REQUIRE(slice_equals(&s1, &s1_z));
    REQUIRE(slice_equals(&s1, &s2));
    REQUIRE_FALSE(slice_equals(&s1, &s3));
    REQUIRE(slice_equals_str_z(&s1, "hello"));
    REQUIRE_FALSE(slice_equals_str_z(&s1, "hell"));
    REQUIRE(slice_equals_str_s(&s4, "hello world", 5));
    REQUIRE(slice_equals(&s1, &s4));
}

TEST_CASE("Mutable Slice creation and equality") {
    char text[]   = "hello";
    char s2_str[] = "hello";
    char s3_str[] = "hellx";
    char s4_str[] = "hello world";

    MutSlice s1   = mut_slice_from_str_s(text, 5);
    MutSlice s1_z = mut_slice_from_str_z(text);
    MutSlice s2   = mut_slice_from_str_s(s2_str, 5);
    MutSlice s3   = mut_slice_from_str_s(s3_str, 5);
    MutSlice s4   = mut_slice_from_str_s(s4_str, 5);

    REQUIRE(s1.length == 5);
    REQUIRE(mut_slice_equals(&s1, &s1_z));
    REQUIRE(mut_slice_equals(&s1, &s2));
    REQUIRE_FALSE(mut_slice_equals(&s1, &s3));
    REQUIRE(mut_slice_equals_str_z(&s1, "hello"));
    REQUIRE_FALSE(mut_slice_equals_str_z(&s1, "hell"));
    REQUIRE(mut_slice_equals_str_s(&s4, s4_str, 5));
    REQUIRE(mut_slice_equals(&s1, &s4));

    text[0] = 'y';
    REQUIRE(mut_slice_equals_str_z(&s1, "yello"));

    Slice const_slice1 = slice_from_str_s(text, 5);
    REQUIRE(mut_slice_equals_slice(&s1, &const_slice1));
    REQUIRE_FALSE(mut_slice_equals_slice(&s2, &const_slice1));
}

TEST_CASE("Align up integer pointer values") {
    REQUIRE(align_up(0, 8) == 0);
    REQUIRE(align_up(1, 8) == 8);
    REQUIRE(align_up(8, 8) == 8);
    REQUIRE(align_up(9, 8) == 16);

    REQUIRE(align_up(17, 16) == 32);
    REQUIRE(align_up(32, 16) == 32);
}

TEST_CASE("Align pointers correctly") {
    char  buffer[64];
    void* base = buffer;

    void* aligned_8  = align_ptr(base, 8);
    void* aligned_16 = align_ptr(base, 16);

    REQUIRE(((uintptr_t)aligned_8 % 8) == 0);
    REQUIRE(((uintptr_t)aligned_16 % 16) == 0);
    REQUIRE((uintptr_t)aligned_16 >= (uintptr_t)base);
}

TEST_CASE("Pointer offset math works correctly") {
    char  buf[10];
    void* base       = buf;
    void* offset_ptr = ptr_offset(base, 5);
    REQUIRE((uintptr_t)offset_ptr == (uintptr_t)base + 5);

    void* back = ptr_offset(offset_ptr, -5);
    REQUIRE(back == base);
}

TEST_CASE("Swap swaps memory content correctly") {
    int a = 10, b = 20;
    swap(&a, &b, sizeof(int));
    REQUIRE(a == 20);
    REQUIRE(b == 10);

    double x = 1.23, y = 9.87;
    swap(&x, &y, sizeof(double));
    REQUIRE(approx_eq_double(x, 9.87, 1e-6));
    REQUIRE(approx_eq_double(y, 1.23, 1e-6));

    char str1[] = "abc";
    char str2[] = "xyz";
    swap(str1, str2, sizeof(str1));
    REQUIRE(strcmp(str1, "xyz") == 0);
    REQUIRE(strcmp(str2, "abc") == 0);
}

TEST_CASE("String duplication") {
    const char* original = "hello world";

    SECTION("Null terminated strings") {
        char* copy_z = strdup_z(original);
        REQUIRE(copy_z);
        REQUIRE(strcmp(copy_z, original) == 0);

        copy_z[0] = 'H';
        REQUIRE(strcmp(copy_z, "Hello world") == 0);
        REQUIRE(strcmp(original, "hello world") == 0);
        free(copy_z);
    }

    SECTION("Less than full size dupe") {
        char* copy_s = strdup_s(original, 5);
        REQUIRE(copy_s);
        REQUIRE(strncmp(copy_s, original, 5) == 0);
        REQUIRE(copy_s[5] == '\0');
        free(copy_s);
    }

    SECTION("Full size dupe") {
        size_t len    = strlen(original);
        char*  copy_s = strdup_s(original, len);
        REQUIRE(copy_s);
        REQUIRE(strcmp(copy_s, original) == 0);
        free(copy_s);
    }

    SECTION("Empty string") {
        const char* empty  = "";
        char*       copy_z = strdup_z(empty);
        REQUIRE(copy_z);
        REQUIRE(strcmp(copy_z, "") == 0);
        free(copy_z);

        char* copy_s = strdup_s(empty, 0);
        REQUIRE(copy_s);
        REQUIRE(strcmp(copy_s, "") == 0);
        free(copy_s);
    }
}
