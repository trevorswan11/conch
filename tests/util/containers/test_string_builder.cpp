#include "catch_amalgamated.hpp"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "util/containers/string_builder.h"
}

TEST_CASE("StringBuilder basic append") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 4));

    REQUIRE(string_builder_append(&sb, 'h'));
    REQUIRE(string_builder_append(&sb, 'i'));
    MutSlice slice = string_builder_to_string(&sb);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hi") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder append many") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2));

    const char* text = "hello";
    REQUIRE(string_builder_append_many(&sb, (char*)text, 5));

    MutSlice slice = string_builder_to_string(&sb);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "hello") == 0);

    slice.ptr[0] = 'H';
    REQUIRE(strcmp(slice.ptr, "Hello") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder multiple appends") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2));

    REQUIRE(string_builder_append(&sb, 'A'));
    REQUIRE(string_builder_append_many(&sb, (char*)"BC", 2));
    REQUIRE(string_builder_append(&sb, 'D'));

    MutSlice slice = string_builder_to_string(&sb);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "ABCD") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder empty initialization") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 1));

    MutSlice slice = string_builder_to_string(&sb);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}

TEST_CASE("StringBuilder handles null input in append_many") {
    StringBuilder sb;
    REQUIRE(string_builder_init(&sb, 2));

    REQUIRE_FALSE(string_builder_append_many(&sb, NULL, 5));
    MutSlice slice = string_builder_to_string(&sb);
    REQUIRE(slice.ptr);
    REQUIRE(strcmp(slice.ptr, "") == 0);

    free(slice.ptr);
}
