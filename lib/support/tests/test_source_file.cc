#include <array>
#include <ranges>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "diagnostic.hh"
#include "option.hh"
#include "source_file.hh"
#include "types.hh"

namespace ghoti::tests {

// clang-format off
constexpr std::string_view source{
    R"(This is line 1
This is line 2
    This is line 3 that starts with spaces
)"};
// clang-format on

namespace {

auto test_diag_strings(const SourceLocation&         t,
                       std::string_view              expected_line,
                       opt::Option<std::string_view> expected_caret) {
    const SourceFile file{source};
    const auto [ln, caret]{file.get_diagnostic_strings(t)};

    CHECK(ln == expected_line);
    if (expected_caret) {
        REQUIRE(caret);
        CHECK(*caret == *expected_caret);
    } else {
        CHECK_FALSE(caret);
    }
}

} // namespace

TEST_CASE("Offset generation") {
    LineOffsets offsets{source};
    CHECK(offsets.size() == 4);

    constexpr std::array expected_mappings{0UZ, 15UZ, 30UZ, 73UZ};
    for (const auto& [offset, expected] : std::views::zip(offsets, expected_mappings)) {
        CHECK(offset == expected);
    }
}

TEST_CASE("First and second line diagnostics") {
    constexpr std::array lines{"This is line 1", "This is line 2"};
    for (usize i{0}; i < lines.size(); ++i) {
        test_diag_strings({i, 0UZ}, lines[i], "^");
        test_diag_strings({i, 1UZ}, lines[i], " ^");
        test_diag_strings({i, 5UZ}, lines[i], "     ^");
        test_diag_strings({i, 13UZ}, lines[i], "             ^");
        test_diag_strings({i, 14UZ}, lines[i], "              ^");
        test_diag_strings({i, 17UZ}, lines[i], opt::none);
    }
}

TEST_CASE("Third line diagnostics") {
    constexpr std::string_view line{"This is line 3 that starts with spaces"};
    test_diag_strings({2UZ, 0UZ}, line, opt::none);
    test_diag_strings({2UZ, 4UZ}, line, "^");
    test_diag_strings({2UZ, 5UZ}, line, " ^");
    test_diag_strings({2UZ, 50UZ}, line, opt::none);
}

TEST_CASE("Out of range line diagnostics") {
    test_diag_strings({10UZ, 0UZ}, "<invalid line>", opt::none);
}

} // namespace ghoti::tests
