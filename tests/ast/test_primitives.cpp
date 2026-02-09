#include <catch_amalgamated.hpp>

#include "ast_nodes.hpp"

#include "ast/expressions/primitive.hpp"

using namespace conch;

TEST_CASE("Single-line string parsing") {
    using N = ast::StringExpression;
    helpers::test_primitive<N>(R"("This is a string")", TokenType::STRING, R"(This is a string)");
    helpers::test_primitive<N>(R"("Hello, 'World'!")", TokenType::STRING, R"(Hello, 'World'!)");
    helpers::test_primitive<N>(R"("")", TokenType::STRING, R"()");
}

TEST_CASE("Multi-line string parsing") {
    using N = ast::StringExpression;
    helpers::test_primitive<N>("\\\\This is a string\n;",
                               "This is a string",
                               TokenType::MULTILINE_STRING,
                               "This is a string");
    helpers::test_primitive<N>("\\\\Hello, 'World'!\n\\\\\n;",
                               "Hello, 'World'!\n\\\\",
                               TokenType::MULTILINE_STRING,
                               "Hello, 'World'!\n");
    helpers::test_primitive<N>("\\\\\n;", "", TokenType::MULTILINE_STRING, "");
}

TEST_CASE("Signed integer parsing") {
    using N = ast::SignedIntegerExpression;
    helpers::test_primitive<N>("0", TokenType::INT_10, 0LL);
    helpers::test_primitive<N>("0b10011101101", TokenType::INT_2, 0b10011101101LL);
    helpers::test_primitive<N>("0o1234567", TokenType::INT_8, 342'391LL);
    helpers::test_primitive<N>("0xFF8a91d", TokenType::INT_16, 0xFF8a91dLL);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFFFF", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned integer parsing") {
    using N = ast::UnsignedIntegerExpression;
    helpers::test_primitive<N>("0u", TokenType::UINT_10, 0ULL);
    helpers::test_primitive<N>("0b10011101101u", TokenType::UINT_2, 0b10011101101ULL);
    helpers::test_primitive<N>("0o1234567u", TokenType::UINT_8, 342'391ULL);
    helpers::test_primitive<N>("0xFF8a91du", TokenType::UINT_16, 0xFF8a91dULL);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFF", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Size integer parsing") {
    using N = ast::SizeIntegerExpression;
    helpers::test_primitive<N>("0uz", TokenType::UZINT_10, 0UZ);
    helpers::test_primitive<N>("0b10011101101uz", TokenType::UZINT_2, 0b10011101101UZ);
    helpers::test_primitive<N>("0o1234567uz", TokenType::UZINT_8, 342'391UZ);
    helpers::test_primitive<N>("0xFF8a91duz", TokenType::UZINT_16, 0xFF8a91dUZ);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFF", nullopt, ParserDiagnostic{ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Byte parsing") {
    using N = ast::ByteExpression;
    helpers::test_primitive<N>("'3'", TokenType::BYTE, '3');
    helpers::test_primitive<N>("'\\0'", TokenType::BYTE, '\0');

    helpers::test_primitive<N>(
        "'\\f'", {}, ParserDiagnostic{ParserError::UNKNOWN_CHARACTER_ESCAPE, 1, 1});
}

TEST_CASE("Floating point parsing") {
    using N = ast::FloatExpression;
    helpers::test_primitive<N>("1023.0", TokenType::FLOAT, 1023.0);
    helpers::test_primitive<N>("1023.234612", TokenType::FLOAT, 1023.234612);
    helpers::test_primitive<N>("1023.234612e234", TokenType::FLOAT, 1023.234612e234);

    helpers::test_primitive<N>(
        "1023.234612e234000", nullopt, ParserDiagnostic{ParserError::FLOAT_OVERFLOW, 1, 1});
}

TEST_CASE("Bool & nil parsing") {
    using N = ast::BoolExpression;
    helpers::test_primitive<N>("true", TokenType::TRUE, true);
    helpers::test_primitive<N>("false", TokenType::FALSE, false);
    helpers::test_primitive<ast::NilExpression>("nil", TokenType::NIL, {});
}
