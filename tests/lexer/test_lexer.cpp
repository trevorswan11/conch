#include <catch_amalgamated.hpp>

#include "lexer/lexer.hpp"

TEST_CASE("Foo") { REQUIRE(Lexer::Foo() == 1); [[maybe_unused]] auto* i = new int; }
