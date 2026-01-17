#include <catch_amalgamated.hpp>

#include <string>

#include "lexer/token.hpp"

TEST_CASE("Promotion of invalid tokens") {
    const std::string input{"1"};
    const Token       tok = {
              .type   = TokenType::INT_10,
              .slice  = input,
              .line   = 0,
              .column = 0,
    };

    const auto promoted = tok.promote();
    REQUIRE_FALSE(promoted);
    REQUIRE(promoted.error() == TokenError::NON_STRING_TOKEN);
}

TEST_CASE("Promotion of standard string literals") {
    SECTION("Normal case") {
        const std::string input{R"("Hello, World!")"};
        const Token       tok = {
                  .type   = TokenType::STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected{"Hello, World!"};
        REQUIRE(expected == *promoted);
    }

    SECTION("Escaped case") {
        const std::string input{R"(""Hello, World!"")"};
        const Token       tok = {
                  .type   = TokenType::STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected{R"("Hello, World!")"};
        REQUIRE(expected == *promoted);
    }

    SECTION("Empty case") {
        const std::string input{R"("")"};
        const Token       tok = {
                  .type   = TokenType::STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected;
        REQUIRE(expected == *promoted);
    }

    SECTION("Malformed case") {
        const std::string input{R"(")"};
        const Token       tok = {
                  .type   = TokenType::STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE_FALSE(promoted);
        REQUIRE(promoted.error() == TokenError::UNEXPECTED_CHAR);
    }
}

TEST_CASE("Promotion of multiline literals") {
    SECTION("Normal case no newline") {
        const std::string input{R"(\\Hello,"World!")"};
        const Token       tok = {
                  .type   = TokenType::MULTILINE_STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected = R"(Hello,"World!")";
        REQUIRE(expected == *promoted);
    }

    SECTION("Normal case newline") {
        const std::string input{"\\\\Hello,\n\\\\World!\n\\\\"};
        const Token       tok = {
                  .type   = TokenType::MULTILINE_STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected = "Hello,\nWorld!\n";
        REQUIRE(expected == *promoted);
    }

    SECTION("Empty case") {
        const std::string input{R"(\\)"};
        const Token       tok = {
                  .type   = TokenType::MULTILINE_STRING,
                  .slice  = input,
                  .line   = 0,
                  .column = 0,
        };

        const auto promoted = tok.promote();
        REQUIRE(promoted);
        const std::string expected;
        REQUIRE(expected == *promoted);
    }
}
