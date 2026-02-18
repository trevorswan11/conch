#include <algorithm>

#include "ast/expressions/struct.hpp"

#include "ast/statements/decl.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

StructExpression::StructExpression(const Token&                    start_token,
                                   std::vector<Box<DeclStatement>> members) noexcept
    : ExprBase{start_token}, members_{std::move(members)} {}

StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (parser.current_token_is(TokenType::PACKED)) { TRY(parser.expect_peek(TokenType::STRUCT)); }

    std::vector<Box<DeclStatement>> members;
    TRY(parser.expect_peek(TokenType::LBRACE));
    while (!parser.peek_token_is(TokenType::RBRACE)) {
        parser.advance();
        auto member = TRY(parser.parse_statement());
        if (!member->is<DeclStatement>()) {
            return make_parser_unexpected(ParserError::INVALID_STRUCT_MEMBER, member->get_token());
        }

        members.emplace_back(downcast<DeclStatement>(std::move(member)));
    }

    TRY(parser.expect_peek(TokenType::RBRACE));
    if (members.empty()) { return make_parser_unexpected(ParserError::EMPTY_STRUCT, start_token); }
    return make_box<StructExpression>(start_token, std::move(members));
}

auto StructExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted     = as<StructExpression>(other);
    const auto  members_eq = std::ranges::equal(
        members_, casted.members_, [](const auto& a, const auto& b) { return *a == *b; });

    return is_packed() == casted.is_packed() && members_eq;
}

} // namespace conch::ast
