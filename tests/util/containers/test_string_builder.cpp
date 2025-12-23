#include "catch_amalgamated.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "util/containers/string_builder.h"
}

TEST_CASE("StringBuilder basic append") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 4)));

    REQUIRE(STATUS_OK(string_builder_append(&sb, 'h')));
    REQUIRE(STATUS_OK(string_builder_append(&sb, 'i')));
    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hi") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder append many") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 2)));

    const char* text = "hello";
    REQUIRE(STATUS_OK(string_builder_append_many(&sb, text, 5)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hello") == 0);

    slice.ptr[0] = 'H';
    REQUIRE(strcmp(slice.ptr, "Hello") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder append slices") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 2)));

    const Slice text = slice_from_str_z("hello");
    REQUIRE(STATUS_OK(string_builder_append_slice(&sb, text)));

    const MutSlice mut_text = mut_slice_from_str_z((char*)", world!");
    REQUIRE(STATUS_OK(string_builder_append_mut_slice(&sb, mut_text)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hello, world!") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder multiple appends") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 2)));

    REQUIRE(STATUS_OK(string_builder_append(&sb, 'A')));
    REQUIRE(STATUS_OK(string_builder_append_many(&sb, "BC", 2)));
    REQUIRE(STATUS_OK(string_builder_append(&sb, 'D')));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "ABCD") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder number appends") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 2)));

    REQUIRE(STATUS_OK(string_builder_append(&sb, 'A')));
    REQUIRE(STATUS_OK(string_builder_append_many(&sb, "BC", 2)));
    REQUIRE(STATUS_OK(string_builder_append(&sb, 'D')));
    REQUIRE(STATUS_OK(string_builder_append_unsigned(&sb, 12032)));
    REQUIRE(STATUS_OK(string_builder_append_str_z(&sb, " EF ")));
    REQUIRE(STATUS_OK(string_builder_append_signed(&sb, -12032)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected = "ABCD12032 EF -12032";
    REQUIRE(expected == slice.ptr);

    free(slice.ptr);
}

TEST_CASE("StringBuilder empty initialization") {
    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 1)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder handles null inputs") {
    StringBuilder sb;
    REQUIRE(string_builder_init(nullptr, 10) == Status::NULL_PARAMETER);
    REQUIRE(string_builder_init(&sb, 0) == Status::EMPTY);

    REQUIRE(STATUS_OK(string_builder_init(&sb, 2)));
    REQUIRE(string_builder_append_many(&sb, nullptr, 5) == Status::NULL_PARAMETER);
    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &slice)));
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}
