#include "catch_amalgamated.hpp"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "util/containers/string_builder.h"
#include "util/error.h"
}

TEST_CASE("StringBuilder basic append") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 4) == AnyError::SUCCESS);

    REQUIRE(string_builder_append(&sb, 'h') == AnyError::SUCCESS);
    REQUIRE(string_builder_append(&sb, 'i') == AnyError::SUCCESS);
    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hi") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder append many") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2) == AnyError::SUCCESS);

    const char* text = "hello";
    REQUIRE(string_builder_append_many(&sb, text, 5) == AnyError::SUCCESS);

    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hello") == 0);

    slice.ptr[0] = 'H';
    REQUIRE(strcmp(slice.ptr, "Hello") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder multiple appends") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2) == AnyError::SUCCESS);

    REQUIRE(string_builder_append(&sb, 'A') == AnyError::SUCCESS);
    REQUIRE(string_builder_append_many(&sb, "BC", 2) == AnyError::SUCCESS);
    REQUIRE(string_builder_append(&sb, 'D') == AnyError::SUCCESS);

    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "ABCD") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder number appends") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2) == AnyError::SUCCESS);

    REQUIRE(string_builder_append(&sb, 'A') == AnyError::SUCCESS);
    REQUIRE(string_builder_append_many(&sb, "BC", 2) == AnyError::SUCCESS);
    REQUIRE(string_builder_append(&sb, 'D') == AnyError::SUCCESS);
    REQUIRE(string_builder_append_size(&sb, 12032) == AnyError::SUCCESS);

    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "ABCD12032") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder empty initialization") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 1) == AnyError::SUCCESS);

    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder handles null inputs") {
    StringBuilder sb;
    REQUIRE(string_builder_init(NULL, 10) == AnyError::NULL_PARAMETER);
    REQUIRE(string_builder_init(&sb, 0) == AnyError::EMPTY);

    REQUIRE(string_builder_init(&sb, 2) == AnyError::SUCCESS);
    REQUIRE(string_builder_append_many(&sb, NULL, 5) == AnyError::NULL_PARAMETER);
    MutSlice slice;
    REQUIRE(string_builder_to_string(&sb, &slice) == AnyError::SUCCESS);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}
