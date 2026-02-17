#include "ast/statements/jump.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto JumpStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    Optional<Box<Expression>> value;
    if (start_token.type != TokenType::CONTINUE && !parser.peek_token_is(TokenType::END) &&
        !parser.peek_token_is(TokenType::SEMICOLON)) {
        parser.advance();
        value = TRY(parser.parse_expression());
    }

    return make_box<JumpStatement>(start_token, std::move(value));
}

} // namespace conch::ast
