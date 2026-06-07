#include <string>

#include <catch2/catch_test_macros.hpp>

#include "helpers/common.hh"
#include "module/error.hh"
#include "module/memory_loader.hh"

namespace ghoti::tests {

TEST_CASE("Correct add/load cycle") {
    const std::string expected_content = "This is a file";
    mod::MemoryLoader loader;
    loader.add("mock", expected_content);

    const auto content = helpers::unwrap(loader.load("mock"));
    CHECK(content == expected_content);
}

TEST_CASE("Overwriting entries in the VFS") {
    mod::MemoryLoader loader;
    loader.add("mock", "This is the bad content");

    const std::string expected_content = "This is the good content";
    loader.add("mock", expected_content);

    const auto content = helpers::unwrap(loader.load("mock"));
    CHECK(content == expected_content);
}

TEST_CASE("Missing path in VFS") {
    mod::MemoryLoader loader;
    const auto        actual = helpers::unwrap_err(loader.load("mock"));

    const mod::Diagnostic expected{"Could not find path 'mock' in virtual file system",
                                   mod::Error::PATH_DOES_NOT_EXIST};
    CHECK(actual == expected);
}

} // namespace ghoti::tests
