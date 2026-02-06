#include <catch_amalgamated.hpp>

#include "ast_nodes.hpp"

using namespace conch;

TEST_CASE("Signed integer parsing") {
    helpers::numbers<i64>("0", TokenType::INT_10, 0);
    helpers::numbers<i64>("0b10011101101", TokenType::INT_2, 0b10011101101);
    helpers::numbers<i64>("0o1234567", TokenType::INT_8, 342391);
    helpers::numbers<i64>("0xFF8a91d", TokenType::INT_16, 0xFF8a91d);
}

TEST_CASE("Floating point parsing") {
    helpers::numbers<f64>("1023.0", TokenType::FLOAT, 1023.0);
    helpers::numbers<f64>("1023.234612", TokenType::FLOAT, 1023.234612);
    helpers::numbers<f64>("1023.234612e234", TokenType::FLOAT, 1023.234612e234);
}
