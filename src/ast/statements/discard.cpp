#include "ast/statements/discard.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto DiscardStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DiscardStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<DiscardStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    TRY(parser.expect_peek(TokenType::ASSIGN));
    parser.advance();
    auto expr = TRY(parser.parse_expression());

    return std::make_unique<DiscardStatement>(start_token, std::move(expr));
}

} // namespace ast
