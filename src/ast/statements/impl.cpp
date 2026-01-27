#include <utility>

#include "ast/statements/impl.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"

#include "lexer/token.hpp"
#include "visitor/visitor.hpp"

namespace ast {

ImplStatement::ImplStatement(const Token&                          start_token,
                             std::unique_ptr<IdentifierExpression> parent,
                             std::unique_ptr<BlockStatement>       implementation) noexcept
    : Statement{start_token}, parent_{std::move(parent)},
      implementation_{std::move(implementation)} {}

ImplStatement::~ImplStatement() = default;

auto ImplStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImplStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<ImplStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    TRY(parser.expect_peek(TokenType::IDENT));
    auto parent_name = TRY(IdentifierExpression::parse(parser));
    TRY(parser.expect_peek(TokenType::LBRACE));

    auto block = TRY(BlockStatement::parse(parser));
    if (block->empty()) {
        return make_parser_unexpected(ParserError::EMPTY_IMPL_BLOCK, start_token);
    }

    return std::make_unique<ImplStatement>(start_token, std::move(parent_name), std::move(block));
}

} // namespace ast
