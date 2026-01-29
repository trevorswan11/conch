#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(Parser& parser) -> Expected<Box<BlockStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::vector<Box<Statement>> statements;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();

        auto inner_stmt = TRY(parser.parse_statement());
        statements.emplace_back(std::move(inner_stmt));
    }

    return make_box<BlockStatement>(start_token, std::move(statements));
}

} // namespace conch::ast
