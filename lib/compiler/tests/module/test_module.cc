#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "helpers/common.hh"
#include "module/error.hh"
#include "module/memory_loader.hh"
#include "module/module.hh"

#include <config.h>

namespace ghoti::tests {

TEST_CASE("Fetching non-relative file modules") {
    mod::MemoryLoader  loader;
    mod::ModuleManager manager{loader};

#if GHOTI_WINDOWS
    const std::string_view file{"C:/fake/foo.gh"};
#else
    const std::string_view file{"/fake/foo.gh"};
#endif
    const auto actual = helpers::unwrap_err(manager.try_get_file_module(file));

    const mod::Diagnostic expected{
        fmt::format("Requested file '{}' is absolute", file),
        mod::Error::MODULE_PATH_NOT_RELATIVE,
    };
    CHECK(actual == expected);
}

TEST_CASE("Fetching missing library modules") {
    mod::MemoryLoader  loader;
    mod::ModuleManager manager{loader};
    const auto         actual = helpers::unwrap_err(manager.try_get_library_module("foo"));

    const mod::Diagnostic expected{
        "Unknown module 'foo'",
        mod::Error::MODULE_DOES_NOT_EXIST,
    };
    CHECK(actual == expected);
}

TEST_CASE("Adding duplicate library module") {
    mod::MemoryLoader  loader;
    mod::ModuleManager manager{loader};
    REQUIRE(manager.add_library_module("foo", "foo.gh"));
    const auto actual = helpers::unwrap_err(manager.add_library_module("foo", "src/foo.gh"));

    const mod::Diagnostic expected{
        "Attempt to add duplicate module 'foo' from path 'src/foo.gh' which already exists at "
        "path 'foo.gh'",
        mod::Error::MODULE_ALREADY_EXISTS,
    };
    CHECK(actual == expected);
}

} // namespace ghoti::tests
