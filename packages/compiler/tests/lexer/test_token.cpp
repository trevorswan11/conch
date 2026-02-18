#include <format>
#include <string>

#include <catch_amalgamated.hpp>

#include "lexer/token.hpp"

using namespace conch;

TEST_CASE("Promotion of invalid tokens") {
    const auto  input{"1"};
    const Token tok{TokenType::INT_10, input, 0, 0};

    const auto promoted = tok.promote();
    REQUIRE_FALSE(promoted);
    REQUIRE(promoted.error() == Diagnostic{TokenError::NON_STRING_TOKEN, 0, 0});
}

TEST_CASE("Promotion of standard string literals") {
    SECTION("Normal case") {
        const auto  input{R"("Hello, World!")"};
        const Token tok = {TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const auto expected{"Hello, World!"};
        REQUIRE(expected == *promoted);
    }

    SECTION("Escaped case") {
        const auto  input{R"(""Hello, World!"")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const auto expected{R"("Hello, World!")"};
        REQUIRE(expected == *promoted);
    }

    SECTION("Empty case") {
        const auto  input{R"("")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected;
        REQUIRE(expected == *promoted);
    }

    SECTION("Malformed case") {
        const auto  input{R"(")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE_FALSE(promoted);
        REQUIRE(promoted.error() == Diagnostic{TokenError::UNEXPECTED_CHAR, 0, 0});
    }
}

TEST_CASE("Promotion of multiline literals") {
    SECTION("Normal case no newline") {
        const auto  input{R"(\\Hello,"World!")"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const auto expected = R"(Hello,"World!")";
        REQUIRE(expected == *promoted);
    }

    SECTION("Normal case newline") {
        const auto  input{"\\\\Hello,\n\\\\World!\n\\\\"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const auto expected = "Hello,\nWorld!\n";
        REQUIRE(expected == *promoted);
    }

    SECTION("Empty case") {
        const auto  input{R"(\\)"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected;
        REQUIRE(expected == *promoted);
    }
}

TEST_CASE("Token formatting") {
    const Token tok{TokenType::STRING, R"("Hello, World!")", 1, 24};

    const auto expected{R"(STRING("Hello, World!") [1, 24])"};
    const auto actual = std::format("{}", tok);
    REQUIRE(expected == actual);
}
