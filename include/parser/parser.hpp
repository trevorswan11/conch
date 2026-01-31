#pragma once

#include <array>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "util/common.hpp"
#include "util/functional.hpp"
#include "util/optional.hpp"

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
    EMPTY_IMPL_BLOCK,
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
};

using ParserDiagnostic = Diagnostic<ParserError>;

template <typename... Args>
auto make_parser_unexpected(Args&&... args) -> Unexpected<ParserDiagnostic> {
    return Unexpected<ParserDiagnostic>{ParserDiagnostic{std::forward<Args>(args)...}};
}

class Parser {
  public:
    using PrefixFn = Thunk<Expected<Box<ast::Expression>, ParserDiagnostic>(Parser&)>;
    using InfixFn =
        Thunk<Expected<Box<ast::Expression>, ParserDiagnostic>(Parser&, Box<ast::Expression>)>;

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance(uint8_t times = 1) noexcept -> const Token&;
    auto consume() -> std::pair<ast::AST, std::span<const ParserDiagnostic>>;

    auto current_token() const noexcept -> const Token& { return current_token_; }
    auto peek_token() const noexcept -> const Token& { return peek_token_; }

    auto current_token_is(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peek_token_is(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    [[nodiscard]] auto expect_current(TokenType expected)
        -> Expected<std::monostate, ParserDiagnostic>;
    [[nodiscard]] auto current_error(TokenType expected) -> ParserDiagnostic {
        return tt_mismatch_error(expected, current_token_);
    }
    [[nodiscard]] auto expect_peek(TokenType expected)
        -> Expected<std::monostate, ParserDiagnostic>;
    [[nodiscard]] auto peek_error(TokenType expected) -> ParserDiagnostic {
        return tt_mismatch_error(expected, peek_token_);
    }

    auto current_precedence() const noexcept -> Precedence;
    auto peek_precedence() const noexcept -> Precedence;

    [[nodiscard]] auto parse_statement() -> Expected<Box<ast::Statement>, ParserDiagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> Expected<Box<ast::Expression>, ParserDiagnostic>;

    static auto poll_prefix(TokenType tt) noexcept -> Optional<const PrefixFn&>;
    static auto poll_infix(TokenType tt) noexcept -> Optional<const InfixFn&>;

  private:
    static auto tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic;

    using PrefixPair = std::pair<TokenType, PrefixFn>;
    static std::array<PrefixPair, 34> PREFIX_FNS;

    using InfixPair = std::pair<TokenType, InfixFn>;
    static std::array<InfixPair, 39> INFIX_FNS;

  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};

    std::vector<ParserDiagnostic> diagnostics_{};
};

} // namespace conch
