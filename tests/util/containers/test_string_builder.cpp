#include "catch_amalgamated.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "fixtures.hpp"

extern "C" {
#include "util/containers/string_builder.h"
}

TEST_CASE("StringBuilder basic append") {
    SBFixture      sbf{4};
    StringBuilder* sb = sbf.sb();

    REQUIRE(STATUS_OK(string_builder_append(sb, 'h')));
    REQUIRE(STATUS_OK(string_builder_append(sb, 'i')));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected = "hi";
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder append many") {
    SBFixture      sbf{2};
    StringBuilder* sb = sbf.sb();

    const char* text = "hello";
    REQUIRE(STATUS_OK(string_builder_append_many(sb, text, 5)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);
    std::string expected = "hello";
    REQUIRE(expected == slice.ptr);

    expected     = "Hello";
    slice.ptr[0] = 'H';
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder append slices") {
    SBFixture      sbf{2};
    StringBuilder* sb = sbf.sb();

    const Slice text = slice_from_str_z("hello");
    REQUIRE(STATUS_OK(string_builder_append_slice(sb, text)));

    CStringFixture str{", world!"};
    const MutSlice mut_text = mut_slice_from_str_z(str.raw());
    REQUIRE(STATUS_OK(string_builder_append_mut_slice(sb, mut_text)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected = "hello, world!";
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder multiple appends") {
    SBFixture      sbf{2};
    StringBuilder* sb = sbf.sb();

    REQUIRE(STATUS_OK(string_builder_append(sb, 'A')));
    REQUIRE(STATUS_OK(string_builder_append_many(sb, "BC", 2)));
    REQUIRE(STATUS_OK(string_builder_append(sb, 'D')));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected = "ABCD";
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder number appends") {
    SBFixture      sbf{2};
    StringBuilder* sb = sbf.sb();

    REQUIRE(STATUS_OK(string_builder_append(sb, 'A')));
    REQUIRE(STATUS_OK(string_builder_append_many(sb, "BC", 2)));
    REQUIRE(STATUS_OK(string_builder_append(sb, 'D')));
    REQUIRE(STATUS_OK(string_builder_append_unsigned(sb, 12032)));
    REQUIRE(STATUS_OK(string_builder_append_str_z(sb, " EF ")));
    REQUIRE(STATUS_OK(string_builder_append_signed(sb, -12032)));

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected = "ABCD12032 EF -12032";
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder empty initialization") {
    SBFixture      sbf{1};
    StringBuilder* sb = sbf.sb();

    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected;
    REQUIRE(expected == slice.ptr);
}

TEST_CASE("StringBuilder handles null inputs") {
    StringBuilder bad_sb;
    REQUIRE(string_builder_init(nullptr, 10) == Status::NULL_PARAMETER);
    REQUIRE(string_builder_init(&bad_sb, 0) == Status::EMPTY);

    SBFixture      sbf{2};
    StringBuilder* sb = sbf.sb();
    REQUIRE(string_builder_append_many(sb, nullptr, 5) == Status::NULL_PARAMETER);
    MutSlice slice;
    REQUIRE(STATUS_OK(string_builder_to_string(sb, &slice)));
    REQUIRE(slice.ptr);

    std::string expected;
    REQUIRE(expected == slice.ptr);
}
