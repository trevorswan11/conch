#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/statements/jump.hpp"

#include "lexer/keywords.hpp"

namespace conch::tests {

TEST_CASE("Expressionless jumps") {
    helpers::test_stmt("continue;", ast::JumpStatement{Token{keywords::CONTINUE}, {}});
    helpers::test_stmt("break;", ast::JumpStatement{Token{keywords::BREAK}, {}});
    helpers::test_stmt("return;", ast::JumpStatement{Token{keywords::RETURN}, {}});
}

TEST_CASE("Expression returns") {
    helpers::test_stmt("return 4;",
                       ast::JumpStatement{Token{keywords::RETURN},
                                          make_box<ast::SignedIntegerExpression>(
                                              Token{TokenType::INT_10, "4"}, 4)});

    std::vector<ast::Enumeration> enumerations;
    enumerations.emplace_back(ast::Enumeration{
        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "RED"}), nullopt});
    helpers::test_stmt("return enum { RED };",
                       ast::JumpStatement{Token{keywords::RETURN},
                                          make_box<ast::EnumExpression>(Token{keywords::ENUM},
                                                                        nullopt,
                                                                        std::move(enumerations))});
}

TEST_CASE("Incorrectly terminated jumps") {
    const auto inputs = std::to_array<std::string_view>({"return", "continue", "break"});
    for (const auto& input : inputs) {
        helpers::test_fail(input,
                           ParserDiagnostic{"Expected token SEMICOLON, found END",
                                            ParserError::UNEXPECTED_TOKEN,
                                            1,
                                            input.size() + 1});
    }

    helpers::test_fail("return return",
                       ParserDiagnostic{"No prefix parse function for RETURN(return) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        8});
}

TEST_CASE("Illegal control flow") {
    helpers::test_fail(
        "continue 4;",
        ParserDiagnostic{
            "Expected token SEMICOLON, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 10});

    helpers::test_fail(
        "break 4;",
        ParserDiagnostic{
            "Expected token SEMICOLON, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 7});
}

} // namespace conch::tests
