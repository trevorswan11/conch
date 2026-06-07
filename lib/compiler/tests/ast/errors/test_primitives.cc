#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

namespace {

[[nodiscard]] constexpr auto overflow_error(syntax::Error error) -> syntax::Diagnostic {
    return syntax::Diagnostic{"Overflow of literal", error, 0, 0};
}

} // namespace

TEST_CASE("Signed integer overflow") {
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFFF;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFFFl;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFz;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
}

TEST_CASE("Unsigned integer overflow") {
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFu;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFul;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFuz;",
                              overflow_error(syntax::Error::INTEGER_OVERFLOW));
    helpers::test_parser_fail("'\\f';",
                              syntax::Diagnostic{syntax::Error::UNKNOWN_CHARACTER_ESCAPE, 0, 0});
}

TEST_CASE("Floating point overflow") {
    helpers::test_parser_fail("1023.234612e234000f;",
                              overflow_error(syntax::Error::FLOAT_OVERFLOW));
    helpers::test_parser_fail("1023.234612e234000;",
                              overflow_error(syntax::Error::DOUBLE_OVERFLOW));
}

} // namespace ghoti::tests
