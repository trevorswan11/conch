#pragma once

#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "memory.hpp"

#include "parser/precedence.hpp"

#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

namespace conch::ast {

class Node;
class Statement;
class Expression;

using AST = std::vector<Box<Node>>;

} // namespace conch::ast

namespace conch {

enum class ParserError : u8 {
    UNEXPECTED_TOKEN,
    ENUM_MISSING_VARIANTS,
    MISSING_TRAILING_COMMA,
    MISSING_PREFIX_PARSER,
    INFIX_MISSING_RHS,
    ILLEGAL_IDENTIFIER,
    END_OF_TOKEN_STREAM,
    CONST_DECL_MISSING_VALUE,
    FORWARD_VAR_DECL_MISSING_TYPE,
    EMPTY_USER_IMPORT,
    ILLEGAL_IMPORT_TYPE,
    USER_IMPORT_MISSING_ALIAS,
    DUPLICATE_DECL_MODIFIER,
    ILLEGAL_DECL_MODIFIERS,
    INTEGER_OVERFLOW,
    MALFORMED_INTEGER,
    FLOAT_OVERFLOW,
    MALFORMED_FLOAT,
    UNKNOWN_CHARACTER_ESCAPE,
    MALFORMED_CHARACTER,
    MALFORMED_STRING,
    PREFIX_MISSING_OPERAND,
    INDEX_MISSING_EXPRESSION,
    DISCARD_MISSING_DISCARDEE,
    ILLEGAL_BLOCK_STATEMENT,
    EMPTY_LOOP,
    WHILE_MISSING_CONDITION,
    INVALID_STRUCT_MEMBER,
    PACKED_AFTER_STRUCT_KEYWORD,
    EMPTY_STRUCT,
    EXTERN_VALUE_INITIALIZED,
    EXTERN_MISSING_TYPE,
    ILLEGAL_LOOP_NON_BREAK,
    ILLEGAL_FOR_LOOP_CAPTURE,
    EMPTY_FOR_LOOP,
    FOR_ITERABLE_CAPTURE_MISMATCH,
    ILLEGAL_FOR_LOOP_DISCARD,
    EMPTY_WHILE_CONTINUATION,
    EMPTY_WHILE_LOOP,
    IF_MISSING_CONDITION,
    ILLEGAL_IF_BRANCH,
    MISSING_ARRAY_SIZE_TOKEN,
    ILLEGAL_ARRAY_SIZE_TYPE,
    EMPTY_ARRAY,
    EXPLICIT_ZERO_ARRAY_SIZE,
    EXPLICIT_ARRAY_SIZE_MISMATCH,
    MATCH_EXPR_MISSING_CONDITION,
    ARMLESS_MATCH_EXPR,
    ILLEGAL_MATCH_ARM,
    ILLEGAL_MATCH_CATCH_ALL,
    FUNCTION_PARAMETER_HAS_DEFAULT_VALUE,
    ILLEGAL_FUNCTION_DEFINITION,
    ILLEGAL_TYPE_MODIFIER,
    ILLEGAL_EXPLICIT_TYPE,
    EXPLICIT_FN_TYPE_HAS_BODY,
    ILLEGAL_OUTER_SCOPE_TYPE,
    ILLEGAL_FUNCTION_TYPE_MODIFIER,
    ILLEGAL_RETURN_TYPE_MODIFIER,
};

using ParserDiagnostic = Diagnostic<ParserError>;

template <typename... Args>
auto make_parser_unexpected(Args&&... args) -> Unexpected<ParserDiagnostic> {
    return Unexpected<ParserDiagnostic>{ParserDiagnostic{std::forward<Args>(args)...}};
}

class Parser {
  public:
    using Diagnostics = std::vector<ParserDiagnostic>;
    using PrefixFn    = Expected<Box<ast::Expression>, ParserDiagnostic> (*)(Parser&);
    using InfixFn     = Expected<Box<ast::Expression>, ParserDiagnostic> (*)(Parser&,
                                                                         Box<ast::Expression>);

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;

    // Advances the parser, returning the resulting current token.
    // This is a no-op at end of stream.
    auto advance(uint8_t times = 1) noexcept -> const Token&;
    auto consume() -> std::pair<ast::AST, Diagnostics>;

    auto current_token() const noexcept -> const Token& { return current_token_; }
    auto peek_token() const noexcept -> const Token& { return peek_token_; }

    auto current_token_is(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peek_token_is(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    // Advances the cursor tokens only if the expected token type matches the actual current token.
    [[nodiscard]] auto expect_current(TokenType expected)
        -> Expected<std::monostate, ParserDiagnostic>;

    // Indiscriminately returns an error citing the current token.
    [[nodiscard]] auto current_error(TokenType expected) -> ParserDiagnostic {
        return tt_mismatch_error(expected, current_token_);
    }

    // Advances the cursor tokens only if the expected token type matches the actual peek token.
    [[nodiscard]] auto expect_peek(TokenType expected)
        -> Expected<std::monostate, ParserDiagnostic>;

    // Indiscriminately returns an error citing the peek token.
    [[nodiscard]] auto peek_error(TokenType expected) -> ParserDiagnostic {
        return tt_mismatch_error(expected, peek_token_);
    }

    auto poll_current_precedence() const noexcept -> Precedence;
    auto poll_peek_precedence() const noexcept -> Precedence;

    [[nodiscard]] auto parse_statement() -> Expected<Box<ast::Statement>, ParserDiagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> Expected<Box<ast::Expression>, ParserDiagnostic>;

    // Assumes that the current token is looking at the start of the expression.
    // The resulting statement can only be a jump, block, or expression statement.
    [[nodiscard]] auto parse_restricted_statement(ParserError error)
        -> Expected<Box<ast::Statement>, ParserDiagnostic>;

    // Parses a restricted statement only if an else token is currently looked at.
    [[nodiscard]] auto try_parse_restricted_alternate(ParserError error)
        -> Expected<Optional<Box<ast::Statement>>, ParserDiagnostic>;

    static constexpr auto poll_prefix_fn(TokenType tt) noexcept -> Optional<const PrefixFn&>;
    static constexpr auto poll_infix_fn(TokenType tt) noexcept -> Optional<const InfixFn&>;

  private:
    static auto tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic;

  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};
};

} // namespace conch
