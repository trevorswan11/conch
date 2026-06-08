#include <string>
#include <string_view>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "string.hh"

namespace ghoti::tests {

TEST_CASE("String traits") {
    STATIC_CHECK(traits::StringLike<std::string>);
    STATIC_CHECK(traits::StringLike<std::string_view>);
    STATIC_CHECK(traits::StringLike<const char*>);
}

TEST_CASE("Byte type requirement") {
    STATIC_CHECK(std::is_same_v<std::string::value_type, char>);
    STATIC_CHECK(std::is_same_v<char, char>);
}

TEST_CASE("Left trim spaces") {
    CHECK(string::trim_left("") == "");
    CHECK(string::trim_left("the") == "the");
    CHECK(string::trim_left("    the") == "the");
    CHECK(string::trim_left("        ") == "");
}

TEST_CASE("Right trim spaces") {
    CHECK(string::trim_right("") == "");
    CHECK(string::trim_right("the") == "the");
    CHECK(string::trim_right("the    ") == "the");
    CHECK(string::trim_right("        ") == "");
}

TEST_CASE("Trim spaces") {
    CHECK(string::trim("") == "");
    CHECK(string::trim("the") == "the");
    CHECK(string::trim("the    ") == "the");
    CHECK(string::trim("    the") == "the");
    CHECK(string::trim("    the    ") == "the");
    CHECK(string::trim("        ") == "");
}

TEST_CASE("Trim pred") {
    constexpr std::string_view against{"asdaefae"};
    CHECK(string::trim("theasdaefae", [&](char c) -> bool { return against.contains(c); }) == "th");
}

TEST_CASE("String view substrings") {
    constexpr std::string_view str{"abcdefghijk"};
    CHECK(string::substr(str, 2) == "cdefghijk");
    CHECK(string::substr(str, 2, 7) == "cdefghi");
    CHECK(string::substr(str, 100) == "");
}

TEST_CASE("Blank string check") {
    CHECK_FALSE(string::is_blank("const T &text"));
    CHECK(string::is_blank("        "));
    CHECK(string::is_blank("        \t\n\r"));
}

TEST_CASE("String view conversion") {
    const auto*            c_str = "Hello, World!";
    const std::string_view view  = c_str;
    const std::string      str   = c_str;

    CHECK(string::to_view(c_str) == string::to_view(c_str));
    CHECK(string::to_view(c_str) == string::to_view(view));
    CHECK(string::to_view(c_str) == string::to_view(str));
    CHECK(string::to_view(view) == string::to_view(str));
}

} // namespace ghoti::tests
