#include "ast/statements/jump.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto JumpStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<JumpStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    Optional<std::unique_ptr<Expression>> value;
    if (start_token.type != TokenType::CONTINUE && !parser.peek_token_is(TokenType::END) &&
        !parser.peek_token_is(TokenType::SEMICOLON)) {
        parser.advance();
        value = TRY(parser.parse_expression());
    }

    return std::make_unique<JumpStatement>(start_token, std::move(value));
}

} // namespace ast
