#include "ast/expressions/do_while.hpp"

#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

DoWhileLoopExpression::DoWhileLoopExpression(const Token&        start_token,
                                             Box<BlockStatement> block,
                                             Box<Expression>     condition) noexcept
    : ExprBase{start_token}, block_{std::move(block)}, condition_{std::move(condition)} {}

DoWhileLoopExpression::~DoWhileLoopExpression() = default;

auto DoWhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DoWhileLoopExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType::LBRACE));

    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    if (block->empty()) { return make_parser_unexpected(ParserError::EMPTY_LOOP, start_token); }

    TRY(parser.expect_peek(TokenType::WHILE));
    TRY(parser.expect_peek(TokenType::LPAREN));

    if (parser.peek_token_is(TokenType::RPAREN)) {
        return make_parser_unexpected(ParserError::WHILE_MISSING_CONDITION, parser.current_token());
    }
    parser.advance();

    // There's no continuation or non break clause so this is easy :)
    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(TokenType::RPAREN));
    return make_box<DoWhileLoopExpression>(start_token, std::move(block), std::move(condition));
}

auto DoWhileLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<DoWhileLoopExpression>(other);
    return *block_ == *casted.block_ && *condition_ == *casted.condition_;
}

} // namespace conch::ast
