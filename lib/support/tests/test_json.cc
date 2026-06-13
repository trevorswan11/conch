#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <string>

#include "json.hh"

namespace ghoti::tests {

TEST_CASE("Standard alphanum string") {
    CHECK(fmt::format("{}", json::SanitizedString{""}).empty());
    auto result{fmt::format("{}", json::SanitizedString{"my_function_123"})};
    CHECK(result == "my_function_123");
}

TEST_CASE("Quotes and backslashes") {
    auto result{fmt::format("{}", json::SanitizedString{R"(path\to\"file")"})};
    CHECK(result == R"(path\\to\\\"file\")");
}

TEST_CASE("Standard whitespace control characters") {
    auto result = fmt::format("{}", json::SanitizedString{"Line1\nLine2\tTabbed"});
    CHECK(result == R"(Line1\nLine2\tTabbed)");
}

TEST_CASE("Non-printable control characters") {
    auto result{fmt::format("{}", json::SanitizedString{"s\u0002e"})};
    CHECK(result == R"(s\u0002e)");
}

} // namespace ghoti::tests
