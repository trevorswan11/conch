#include "catch_amalgamated.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "fixtures.hpp"

extern "C" {
#include "util/math.h"
#include "util/memory.h"
}

TEST_CASE("Word size determined correctly") {
#ifdef WORD_SIZE_64
    REQUIRE(sizeof(size_t) == 8);
#elif defined(WORD_SIZE_32)
    REQUIRE(sizeof(size_t) == 4);
#endif
}

TEST_CASE("Zeroed slices") {
    const Slice       slice         = zeroed_slice();
    const MutSlice    mut_slice     = zeroed_mut_slice();
    const AnySlice    any_slice     = zeroed_any_slice();
    const AnyMutSlice any_mut_slice = zeroed_any_mut_slice();

    const auto test_zeroed_any = [](AnySlice any) {
        REQUIRE_FALSE(any.ptr);
        REQUIRE(any.length == 0);
    };

    test_zeroed_any(any_from_slice(&slice));
    test_zeroed_any(any_from_mut_slice(&mut_slice));
    test_zeroed_any(any_from_any_mut_slice(&any_mut_slice));
}

TEST_CASE("Slice creation and equality") {
    const char* text = "hello";
    const Slice s1   = slice_from_str_s(text, 5);
    const Slice s1_z = slice_from_str_z(text);
    const Slice s2   = slice_from_str_s("hello", 5);
    const Slice s3   = slice_from_str_s("hellx", 5);
    const Slice s4   = slice_from_str_s("hello world", 5);

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

    const MutSlice s1   = mut_slice_from_str_s(text, 5);
    const MutSlice s1_z = mut_slice_from_str_z(text);
    const MutSlice s2   = mut_slice_from_str_s(s2_str, 5);
    const MutSlice s3   = mut_slice_from_str_s(s3_str, 5);
    const MutSlice s4   = mut_slice_from_str_s(s4_str, 5);

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

    const Slice const_slice1 = slice_from_str_s(text, 5);
    REQUIRE(mut_slice_equals_slice(&s1, &const_slice1));
    REQUIRE_FALSE(mut_slice_equals_slice(&s2, &const_slice1));
}

TEST_CASE("Slice edge cases") {
    const MutSlice empty_mut = {nullptr, 0};
    REQUIRE(mut_slice_equals_str_z(&empty_mut, nullptr));
    REQUIRE(mut_slice_equals_str_s(&empty_mut, nullptr, 0));

    const Slice empty = slice_from_mut(&empty_mut);
    REQUIRE(slice_equals_str_z(&empty, nullptr));
    REQUIRE(slice_equals_str_s(&empty, nullptr, 0));
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

    const void* aligned_8  = align_ptr(base, 8);
    const void* aligned_16 = align_ptr(base, 16);

    REQUIRE(((uintptr_t)aligned_8 % 8) == 0);
    REQUIRE(((uintptr_t)aligned_16 % 16) == 0);
    REQUIRE((uintptr_t)aligned_16 >= (uintptr_t)base);
}

TEST_CASE("Pointer offset math works correctly") {
    char  buf[10];
    void* base       = buf;
    void* offset_ptr = ptr_offset(base, 5);
    REQUIRE((uintptr_t)offset_ptr == (uintptr_t)base + 5);

    const void* back = ptr_offset(offset_ptr, -5);
    REQUIRE(back == base);
}

TEST_CASE("Swap swaps memory content correctly") {
    int a = 10;
    int b = 20;
    swap(&a, &b, sizeof(int));
    REQUIRE(a == 20);
    REQUIRE(b == 10);

    swap(&a, nullptr, sizeof(int));
    REQUIRE(a == 20);
    swap(nullptr, &b, sizeof(int));
    REQUIRE(b == 10);

    double x = 1.23;
    double y = 9.87;
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
        char*                copy_z = strdup_z(original); // NOLINT
        const Fixture<char*> strf(copy_z);

        REQUIRE(copy_z);
        REQUIRE(strcmp(copy_z, original) == 0);

        copy_z[0] = 'H';
        REQUIRE(strcmp(copy_z, "Hello world") == 0);
        REQUIRE(strcmp(original, "hello world") == 0);
    }

    SECTION("Less than full size dupe") {
        char*                copy_s = strdup_s(original, 5); // NOLINT
        const Fixture<char*> strf(copy_s);

        REQUIRE(copy_s);
        REQUIRE(strncmp(copy_s, original, 5) == 0);
        REQUIRE(copy_s[5] == '\0');
    }

    SECTION("Full size dupe") {
        const size_t         len    = strlen(original);
        char*                copy_s = strdup_s(original, len); // NOLINT
        const Fixture<char*> strf(copy_s);

        REQUIRE(copy_s);
        REQUIRE(strcmp(copy_s, original) == 0);
    }

    SECTION("Empty string") {
        const char*          empty  = "";
        char*                copy_z = strdup_z(empty); // NOLINT
        const Fixture<char*> strfz(copy_z);
        REQUIRE(copy_z);
        REQUIRE(strcmp(copy_z, "") == 0);

        char*                copy_s = strdup_s(empty, 0); // NOLINT
        const Fixture<char*> strfs(copy_s);
        REQUIRE(copy_s);
        REQUIRE(strcmp(copy_s, "") == 0);
    }

    SECTION("Null string") {
        REQUIRE_FALSE(strdup_z_allocator(nullptr, standard_allocator.memory_alloc));
        REQUIRE_FALSE(strdup_s_allocator(nullptr, 0, standard_allocator.memory_alloc));
    }
}

TEST_CASE("Reference counting") {
    struct Int {
        RcControlBlock rc_control;
        int            value;
        int*           heap;
    };

    const auto int_dtor = [](void* i, free_alloc_fn free_alloc) {
        Int* ii = static_cast<Int*>(i);
        if (ii->heap) {
            free_alloc(ii->heap);
            ii->heap = nullptr;
        }
    };

    const auto int_ctor = [&int_dtor](int v, bool heaped) {
        Int* i = static_cast<Int*>(malloc(sizeof(Int)));

        int* j = nullptr;
        if (heaped) {
            j  = static_cast<int*>(malloc(sizeof(int)));
            *j = v;
            *i = {.rc_control = rc_init(int_dtor), .value = v, .heap = j};
        } else {
            *i = {.rc_control = rc_init(nullptr), .value = v, .heap = j};
        }

        return i;
    };

    SECTION("Standard usage") {
        Int* i = int_ctor(2, true);
        REQUIRE(i->rc_control.ref_count == 1);
        Int* i_ref = static_cast<Int*>(rc_retain(i));
        REQUIRE(i->rc_control.ref_count == 2);

        rc_release(i_ref, free);
        REQUIRE(i->rc_control.ref_count == 1);
        rc_release(i, free);
    }

    SECTION("Without destructor") {
        Int* i = int_ctor(2, false);
        REQUIRE(i->rc_control.ref_count == 1);
        Int* i_ref = static_cast<Int*>(rc_retain(i));
        REQUIRE(i->rc_control.ref_count == 2);

        rc_release(i_ref, free);
        REQUIRE(i->rc_control.ref_count == 1);
        rc_release(i, free);
    }

    SECTION("Null release") { rc_release(nullptr, free); }
}
