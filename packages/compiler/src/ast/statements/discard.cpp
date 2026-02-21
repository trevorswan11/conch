#include "ast/statements/discard.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

auto DiscardStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DiscardStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    TRY(parser.expect_peek(TokenType::ASSIGN));
    parser.advance();
    auto expr = TRY(parser.parse_expression());

    TRY(parser.expect_peek(TokenType::SEMICOLON));
    return make_box<DiscardStatement>(start_token, std::move(expr));
}

} // namespace conch::ast
