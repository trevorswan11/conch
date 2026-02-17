#include "ast_nodes.hpp"
#include "ast/expressions/identifier.hpp"

namespace helpers {

auto test_ident(std::string_view input, Optional<TokenType> expected_type) -> void {
    Parser p{input};
    auto   parse_result = p.consume();

    REQUIRE(parse_result.second.empty());
    REQUIRE(parse_result.first.size() == 1);

    auto&       ast = parse_result.first;
    const auto  actual{std::move(ast[0])};
    const auto& expr_stmt = into_expression_statement(*actual);

    const ast::IdentifierExpression& expected{Token{*expected_type, input}, input};
    REQUIRE(expected == expr_stmt.get_expression());
}

} // namespace helpers
