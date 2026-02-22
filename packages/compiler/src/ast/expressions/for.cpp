#include <algorithm>

#include "ast/expressions/for.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/pointer.hpp"
#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

ForLoopExpression::ForLoopExpression(const Token&                                     start_token,
                                     std::vector<Box<Expression>>                     iterables,
                                     Optional<std::vector<Optional<Box<Expression>>>> captures,
                                     Box<BlockStatement>                              block,
                                     Optional<Box<Statement>> non_break) noexcept
    : ExprBase{start_token}, iterables_{std::move(iterables)}, captures_{std::move(captures)},
      block_{std::move(block)}, non_break_{std::move(non_break)} {}

ForLoopExpression::~ForLoopExpression() = default;

auto ForLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Iterables have to be surrounded by parentheses
    TRY(parser.expect_peek(TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(TokenType::RPAREN)) {
        return make_parser_unexpected(ParserError::WHILE_MISSING_CONDITION, start_token);
    }

    std::vector<Box<Expression>> iterables;
    while (!parser.peek_token_is(TokenType::RPAREN) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();

        auto iterable = TRY(parser.parse_expression());
        iterables.emplace_back(std::move(iterable));

        if (!parser.peek_token_is(TokenType::RPAREN)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }

    TRY(parser.expect_peek(TokenType::RPAREN));

    // Captures are entirely optional and take on the zig capture syntax
    Optional<std::vector<Optional<Box<Expression>>>> captures;
    if (parser.peek_token_is(TokenType::OR)) {
        captures.emplace();
        parser.advance();

        while (!parser.peek_token_is(TokenType::OR) && !parser.peek_token_is(TokenType::END)) {
            parser.advance();
            if (parser.current_token_is(TokenType::UNDERSCORE)) {
                captures->emplace_back(nullopt);
            } else {
                auto capture = TRY(parser.parse_expression());
                if (!capture->any<IdentifierExpression, PointerExpression>()) {
                    return make_parser_unexpected(ParserError::ILLEGAL_FOR_LOOP_CAPTURE,
                                                  capture->get_token());
                }
                captures->emplace_back(std::move(capture));
            }

            if (!parser.peek_token_is(TokenType::RPAREN)) {
                TRY(parser.expect_peek(TokenType::COMMA));
            }
        }
    }

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(TokenType::LBRACE));
    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    auto non_break =
        TRY(parser.try_parse_restricted_alternate(ParserError::ILLEGAL_LOOP_NON_BREAK));

    // The number of captures must align with the number of iterables
    if (captures && captures->size() != iterables.size()) {
        return make_parser_unexpected(ParserError::FOR_ITERABLE_CAPTURE_MISMATCH, start_token);
    }

    if (block->empty()) { return make_parser_unexpected(ParserError::EMPTY_FOR_LOOP, start_token); }

    return make_box<ForLoopExpression>(start_token,
                                       std::move(iterables),
                                       std::move(captures),
                                       std::move(block),
                                       std::move(non_break));
}

auto ForLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted       = as<ForLoopExpression>(other);
    const auto  iterables_eq = std::ranges::equal(
        iterables_, casted.iterables_, [](const auto& a, const auto& b) { return *a == *b; });
    const auto captures_eq = optional::safe_eq<std::vector<Optional<Box<Expression>>>>(
        captures_, casted.captures_, [](const auto& a_captures, const auto& b_captures) {
            return std::ranges::equal(
                a_captures, b_captures, [](const auto& a_capture, const auto& b_capture) {
                    return optional::unsafe_eq<Expression>(a_capture, b_capture);
                });
        });
    return iterables_eq && captures_eq && block_ == casted.block_ &&
           optional::unsafe_eq<Statement>(non_break_, casted.non_break_);
}

} // namespace conch::ast
