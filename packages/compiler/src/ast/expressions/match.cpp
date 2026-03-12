#include "ast/expressions/match.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"
#include <variant>

namespace conch::ast {

MatchArm::MatchArm(Box<Expression>   pattern,
                   Optional<Capture> capture,
                   Box<Statement>    dispatch) noexcept
    : pattern_{std::move(pattern)}, capture_{std::move(capture)}, dispatch_{std::move(dispatch)} {}
MatchArm::~MatchArm() = default;

auto MatchArm::is_equal(const MatchArm& other) const noexcept -> bool {
    return *pattern_ == *other.pattern_ &&
           optional::safe_eq<Capture>(
               capture_,
               other.capture_,
               [](const Capture& a, const Capture& b) {
                   if (a.index() != b.index()) { return false; }
                   return std::visit(Overloaded{[&a](const Box<IdentifierExpression>& b) {
                                                    return *b ==
                                                           *std::get<Box<IdentifierExpression>>(a);
                                                },
                                                [](const std::monostate&) { return true; }},
                                     b);
               }) &&
           *dispatch_ == *other.dispatch_;
}

auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto MatchExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(TokenType::RPAREN)) {
        return make_parser_unexpected(ParserError::MATCH_EXPR_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(TokenType::RPAREN));

    TRY(parser.expect_peek(TokenType::LBRACE));
    if (parser.peek_token_is(TokenType::RBRACE)) {
        parser.advance();
        return make_parser_unexpected(ParserError::ARMLESS_MATCH_EXPR, start_token);
    }

    std::vector<MatchArm> arms;
    // Current token is either the LBRACE at the start or a comma before parsing
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();

        auto pattern = TRY(parser.parse_expression());
        TRY(parser.expect_peek(TokenType::FAT_ARROW));

        // There is an optional capture for every arm
        Optional<MatchArm::Capture> capture;
        if (parser.peek_token_is(TokenType::BW_OR)) {
            parser.advance();

            // An underscore is equivalent to a lack of capture
            if (parser.peek_token_is(TokenType::UNDERSCORE)) {
                parser.advance();
                capture.emplace(std::monostate{});
            } else {
                TRY(parser.expect_peek(TokenType::IDENT));
                capture.emplace(
                    downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser))));
            }
            TRY(parser.expect_peek(TokenType::BW_OR));
        }

        // The resulting statement must be restricted like an if branch
        parser.advance();
        auto consequence = TRY(parser.parse_restricted_statement(ParserError::ILLEGAL_MATCH_ARM));
        arms.emplace_back(std::move(pattern), std::move(capture), std::move(consequence));
    }

    // Empty match statements aren't ever allowed
    if (arms.empty()) {
        return make_parser_unexpected(ParserError::ARMLESS_MATCH_EXPR, start_token);
    }

    TRY(parser.expect_peek(TokenType::RBRACE));
    auto catch_all =
        TRY(parser.try_parse_restricted_alternate(ParserError::ILLEGAL_MATCH_CATCH_ALL));

    return make_box<MatchExpression>(
        start_token, std::move(condition), std::move(arms), std::move(catch_all));
}

} // namespace conch::ast
