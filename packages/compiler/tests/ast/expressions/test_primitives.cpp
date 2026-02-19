#include <catch_amalgamated.hpp>

#include "ast/expressions/test_primitives.hpp"

#include "ast/expressions/primitive.hpp"

namespace conch::tests {

TEST_CASE("Single-line string parsing") {
    using N = ast::StringExpression;
    helpers::primitive<N>(R"("This is a string")", TokenType::STRING, R"(This is a string)");
    helpers::primitive<N>(R"("Hello, 'World'!")", TokenType::STRING, R"(Hello, 'World'!)");
    helpers::primitive<N>(R"("")", TokenType::STRING, R"()");
}

TEST_CASE("Multi-line string parsing") {
    using N = ast::StringExpression;
    helpers::primitive<N>("\\\\This is a string\n;",
                          "This is a string",
                          TokenType::MULTILINE_STRING,
                          "This is a string");
    helpers::primitive<N>("\\\\Hello, 'World'!\n\\\\\n;",
                          "Hello, 'World'!\n\\\\",
                          TokenType::MULTILINE_STRING,
                          "Hello, 'World'!\n");
    helpers::primitive<N>("\\\\\n;", "", TokenType::MULTILINE_STRING, "");
}

TEST_CASE("Signed integer parsing") {
    using N = ast::SignedIntegerExpression;
    helpers::primitive<N>("0", TokenType::INT_10, 0);
    helpers::primitive<N>("0b10011101101", TokenType::INT_2, 0b10011101101);
    helpers::primitive<N>("0o1234567", TokenType::INT_8, 342'391);
    helpers::primitive<N>("0xFF8a91d", TokenType::INT_16, 0xFF8a91d);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFFFF", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Signed long integer parsing") {
    using N = ast::SignedLongIntegerExpression;
    helpers::primitive<N>("0l", TokenType::LINT_10, 0LL);
    helpers::primitive<N>("0b10011101101l", TokenType::LINT_2, 0b10011101101LL);
    helpers::primitive<N>("0o1234567l", TokenType::LINT_8, 342'391LL);
    helpers::primitive<N>("0xFF8a91dl", TokenType::LINT_16, 0xFF8a91dLL);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFFFFl", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Signed size integer parsing") {
    using N = ast::ISizeIntegerExpression;
    helpers::primitive<N>("0z", TokenType::ZINT_10, 0Z);
    helpers::primitive<N>("0b10011101101z", TokenType::ZINT_2, 0b10011101101Z);
    helpers::primitive<N>("0o1234567z", TokenType::ZINT_8, 342'391Z);
    helpers::primitive<N>("0xFF8a91dz", TokenType::ZINT_16, 0xFF8a91dZ);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFz", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned integer parsing") {
    using N = ast::UnsignedIntegerExpression;
    helpers::primitive<N>("0u", TokenType::UINT_10, 0U);
    helpers::primitive<N>("0b10011101101u", TokenType::UINT_2, 0b10011101101U);
    helpers::primitive<N>("0o1234567u", TokenType::UINT_8, 342'391U);
    helpers::primitive<N>("0xFF8a91du", TokenType::UINT_16, 0xFF8a91dU);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFu", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned long integer parsing") {
    using N = ast::UnsignedLongIntegerExpression;
    helpers::primitive<N>("0ul", TokenType::ULINT_10, 0ULL);
    helpers::primitive<N>("0b10011101101ul", TokenType::ULINT_2, 0b10011101101ULL);
    helpers::primitive<N>("0o1234567ul", TokenType::ULINT_8, 342'391ULL);
    helpers::primitive<N>("0xFF8a91dul", TokenType::ULINT_16, 0xFF8a91dULL);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFFul", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned size integer parsing") {
    using N = ast::USizeIntegerExpression;
    helpers::primitive<N>("0uz", TokenType::UZINT_10, 0UZ);
    helpers::primitive<N>("0b10011101101uz", TokenType::UZINT_2, 0b10011101101UZ);
    helpers::primitive<N>("0o1234567uz", TokenType::UZINT_8, 342'391UZ);
    helpers::primitive<N>("0xFF8a91duz", TokenType::UZINT_16, 0xFF8a91dUZ);

    helpers::primitive<N>(
        "0xFFFFFFFFFFFFFFFFFuz", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Byte parsing") {
    using N = ast::ByteExpression;
    helpers::primitive<N>("'3'", TokenType::BYTE, '3');
    helpers::primitive<N>("'\\0'", TokenType::BYTE, '\0');

    helpers::primitive<N>(
        "'\\f'", {}, ParserDiagnostic{ParserError::UNKNOWN_CHARACTER_ESCAPE, 1, 1});
}

TEST_CASE("Floating point parsing") {
    using N = ast::FloatExpression;
    helpers::primitive<N>("1023.0", TokenType::FLOAT, 1023.0);
    helpers::primitive<N>("1023.234612", TokenType::FLOAT, 1023.234612);
    helpers::primitive<N>("1023.234612e234", TokenType::FLOAT, 1023.234612e234);

    helpers::primitive<N>(
        "1023.234612e234000", nullopt, ParserDiagnostic{ParserError::FLOAT_OVERFLOW, 1, 1});
}

TEST_CASE("Bool parsing") {
    using N = ast::BoolExpression;
    helpers::primitive<N>("true", TokenType::TRUE, true);
    helpers::primitive<N>("false", TokenType::FALSE, false);
}

} // namespace conch::tests
