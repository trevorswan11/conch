#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/infinite_loop.hpp"

namespace conch::tests {

TEST_CASE("Correct infinite loop") {
    helpers::test_expr_stmt(
        "loop { a; };",
        ast::InfiniteLoopExpression{Token{keywords::LOOP},
                                    helpers::make_expr_block_stmt(helpers::ident_from("a"))});
}

TEST_CASE("Empty infinite loop") {
    helpers::test_fail("loop {};", ParserDiagnostic{ParserError::EMPTY_LOOP, 1, 6});
}

} // namespace conch::tests
