#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<BlockStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::vector<std::unique_ptr<Statement>> statements;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();

        auto inner_stmt = TRY(parser.parse_statement());
        statements.emplace_back(std::move(inner_stmt));
    }

    return std::make_unique<BlockStatement>(start_token, std::move(statements));
}

} // namespace ast
