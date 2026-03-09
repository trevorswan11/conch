#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/primitive.hpp"

namespace conch::tests {

TEST_CASE("Basic enums") {
    helpers::test_expr_stmt(
        "enum {A, B, C};",
        ast::EnumExpression{Token{keywords::ENUM},
                            {},
                            helpers::make_vector<ast::Enumeration>(
                                ast::Enumeration{helpers::make_ident("A"), {}},
                                ast::Enumeration{helpers::make_ident("B"), {}},
                                ast::Enumeration{helpers::make_ident("C"), {}})});

    helpers::test_expr_stmt(
        "enum {A = 1, B = T, };",
        ast::EnumExpression{
            Token{keywords::ENUM},
            {},
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{
                    helpers::make_ident("A"),
                    make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "1"}, 1)},
                ast::Enumeration{helpers::make_ident("B"), helpers::make_ident("T")})});
}

TEST_CASE("Underlying type") {
    helpers::test_expr_stmt(
        "enum : ulong {RED = 3u, B, };",
        ast::EnumExpression{Token{keywords::ENUM},
                            helpers::make_ident("ulong"),
                            helpers::make_vector<ast::Enumeration>(
                                ast::Enumeration{helpers::make_ident("RED"),
                                                 make_box<ast::UnsignedIntegerExpression>(
                                                     Token{TokenType::UINT_10, "3u"}, 3u)},
                                ast::Enumeration{helpers::make_ident("B"), {}})});

    helpers::test_expr_stmt(
        "enum : U {A = 0x4uz};",
        ast::EnumExpression{Token{keywords::ENUM},
                            helpers::make_ident("U"),
                            helpers::make_vector<ast::Enumeration>(ast::Enumeration{
                                helpers::make_ident("A"),
                                make_box<ast::USizeIntegerExpression>(
                                    Token{TokenType::UZINT_16, "0x4uz"}, 0x4uz)})});
}

TEST_CASE("Empty enum") {
    helpers::test_fail("enum {};", ParserDiagnostic{ParserError::ENUM_MISSING_VARIANTS, 1, 6});
    helpers::test_fail("enum : T {};", ParserDiagnostic{ParserError::ENUM_MISSING_VARIANTS, 1, 10});
}

TEST_CASE("Illegal underlying type") {
    helpers::test_fail("enum : 4 {A};", ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 8});
    helpers::test_fail(R"(enum : "e" {A};)",
                       ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 8});
}

} // namespace conch::tests
