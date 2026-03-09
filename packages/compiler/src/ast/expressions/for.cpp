#include <algorithm>
#include <variant>

#include "ast/expressions/for.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

ForLoopCapture::Valued::Valued(TypeModifier modifier, Box<IdentifierExpression> name) noexcept
    : modifier_{std::move(modifier)}, name_{std::move(name)} {}
ForLoopCapture::Valued::~Valued() = default;

ForLoopCapture::ForLoopCapture() noexcept : underlying_{std::monostate{}} {}
ForLoopCapture::ForLoopCapture(Valued valued) noexcept : underlying_{Valued{std::move(valued)}} {}
ForLoopCapture::~ForLoopCapture() = default;

auto ForLoopCapture::is_equal(const ForLoopCapture& other) const noexcept -> bool {
    if (is_discarded()) {
        if (!other.is_discarded()) { return false; }
        return true;
    }

    // At this point lhs must be a true capture
    if (!other.is_valued()) { return false; }
    const auto& this_v  = get_valued();
    const auto& other_v = get_valued();
    return this_v.modifier_ == other_v.modifier_ && *this_v.name_ == *other_v.name_;
}

ForLoopExpression::ForLoopExpression(const Token&                 start_token,
                                     std::vector<Box<Expression>> iterables,
                                     std::vector<ForLoopCapture>  captures,
                                     Box<BlockStatement>          block,
                                     Optional<Box<Statement>>     non_break) noexcept
    : ExprBase{start_token}, iterables_{std::move(iterables)}, captures_{std::move(captures)},
      block_{std::move(block)}, non_break_{std::move(non_break)} {}

ForLoopExpression::~ForLoopExpression() = default;

auto ForLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Iterables have to be surrounded by parentheses
    TRY(parser.expect_peek(TokenType::LPAREN));
    if (parser.peek_token_is(TokenType::RPAREN)) {
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

    // Captures take on something similar to zig's capture syntax
    std::vector<ForLoopCapture> captures;
    TRY(parser.expect_peek(TokenType::BW_OR));
    while (!parser.peek_token_is(TokenType::BW_OR) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        if (parser.current_token_is(TokenType::UNDERSCORE)) {
            captures.emplace_back();
        } else {
            // Always check for a modifier and advance past it if present
            const auto modifier = TypeModifier::from_token(parser.current_token());
            if (!modifier.is_value()) { parser.advance(); }

            auto capture = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
            captures.emplace_back(ForLoopCapture::Valued{std::move(modifier), std::move(capture)});
        }

        if (!parser.peek_token_is(TokenType::BW_OR)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }
    TRY(parser.expect_peek(TokenType::BW_OR));

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(TokenType::LBRACE));
    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    auto non_break =
        TRY(parser.try_parse_restricted_alternate(ParserError::ILLEGAL_LOOP_NON_BREAK));

    // The number of captures must align with the number of iterables
    if (captures.size() != iterables.size()) {
        return make_parser_unexpected(ParserError::FOR_ITERABLE_CAPTURE_MISMATCH, start_token);
    }

    if (block->empty()) {
        return make_parser_unexpected(ParserError::EMPTY_FOR_LOOP, block->get_token());
    }

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
    const auto captures_eq = std::ranges::equal(captures_, casted.captures_);
    return iterables_eq && captures_eq && *block_ == *casted.block_ &&
           optional::unsafe_eq<Statement>(non_break_, casted.non_break_);
}

} // namespace conch::ast
