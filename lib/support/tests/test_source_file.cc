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

namespace helpers {

auto test_diag_strings(const SourceLocation&         t,
                       std::string_view              expected_line,
                       opt::Option<std::string_view> expected_caret) {
    const SourceFile file{source};
    const auto [ln, caret] = file.get_diagnostic_strings(t);

    CHECK(ln == expected_line);
    if (expected_caret) {
        REQUIRE(caret);
        CHECK(*caret == *expected_caret);
    } else {
        CHECK_FALSE(caret);
    }
}

} // namespace helpers

TEST_CASE("Offset generation") {
    LineOffsets offsets{source};
    CHECK(offsets.size() == 4);

    constexpr std::array expected_mappings{0uz, 15uz, 30uz, 73uz};
    for (const auto& [offset, expected] : std::views::zip(offsets, expected_mappings)) {
        CHECK(offset == expected);
    }
}

TEST_CASE("First and second line diagnostics") {
    constexpr std::array lines{"This is line 1", "This is line 2"};
    for (usize i = 0; i < lines.size(); ++i) {
        helpers::test_diag_strings({i, 0uz}, lines[i], "^");
        helpers::test_diag_strings({i, 1uz}, lines[i], " ^");
        helpers::test_diag_strings({i, 5uz}, lines[i], "     ^");
        helpers::test_diag_strings({i, 13uz}, lines[i], "             ^");
        helpers::test_diag_strings({i, 14uz}, lines[i], "              ^");
        helpers::test_diag_strings({i, 17uz}, lines[i], opt::none);
    }
}

TEST_CASE("Third line diagnostics") {
    constexpr std::string_view line{"This is line 3 that starts with spaces"};
    helpers::test_diag_strings({2uz, 0uz}, line, opt::none);
    helpers::test_diag_strings({2uz, 4uz}, line, "^");
    helpers::test_diag_strings({2uz, 5uz}, line, " ^");
    helpers::test_diag_strings({2uz, 50uz}, line, opt::none);
}

TEST_CASE("Out of range line diagnostics") {
    helpers::test_diag_strings({10uz, 0uz}, "<invalid line>", opt::none);
}

} // namespace ghoti::tests
