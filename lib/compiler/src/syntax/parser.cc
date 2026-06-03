#include "syntax/parser.hh"

#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "syntax/builtins.hh"
#include "syntax/error.hh"
#include "syntax/keywords.hh"
#include "syntax/precedence.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include "diagnostic.hh"
#include "enum.hh"
#include "fixed/enum_map.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::syntax {

auto Parser::reset(std::string_view input) noexcept -> void {
    ast_.reset();
    input_ = input;
    lexer_.reset(input);
    advance(2);
}

auto Parser::advance(u8 times) noexcept -> const Token& {
    for (u8 i = 0; i < times; ++i) {
        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

auto Parser::consume(ast::AST& ast) -> Diagnostics {
    reset(input_);
    ast.clear();
    ast_.emplace(ast);

    Diagnostics diagnostics;

    while (!current_token_is(TokenType::END)) {
        // Advance through any amount of semicolons
        const auto skip = [](TokenType tt) { return tt == TokenType::SEMICOLON; };
        if (skip(current_token_.type)) { while (skip(advance().type)); }
        if (current_token_is(TokenType::END)) { break; }

        // Comments are entirely discarded from the tree
        if (!current_token_is(TokenType::COMMENT)) {
            auto stmt = parse_statement(true);
            if (stmt) {
                ast.add_root(**stmt);
            } else {
                diagnostics.emplace_back(std::move(stmt.error()));

                // Errors should advance up to next logical end to prevent useless errors
                const auto stop_condition = [](TokenType tt) {
                    switch (tt) {
                    case TokenType::RBRACE:
                    case TokenType::SEMICOLON:
                    case TokenType::END:       return true;
                    default:                   return false;
                    }
                };
                while (!stop_condition(advance().type));
            }
        }
        advance();
    }

    return diagnostics;
}

auto Parser::expect_peek(TokenType expected) -> Result<void, Diagnostic> {
    if (peek_token_is(expected)) {
        advance();
        return {};
    }
    return Err{peek_error(expected)};
}

auto Parser::expect_semicolon() -> Result<void, Diagnostic> {
    using TokenType::SEMICOLON;
    if (current_token_is(SEMICOLON)) { return {}; }
    return expect_peek(SEMICOLON);
}

auto Parser::peek_error(TokenType expected) -> Diagnostic {
    return Diagnostic{fmt::format("Expected token {}, found {}",
                                  magic_enum::enum_name(expected),
                                  magic_enum::enum_name(peek_token_.type)),
                      Error::UNEXPECTED_TOKEN,
                      peek_token_};
}

auto Parser::get_current_precedence() const noexcept
    -> std::pair<Precedence, opt::Option<Binding>> {
    return Binding::try_get_from(current_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, opt::Option<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, opt::none});
}

auto Parser::get_peek_precedence() const noexcept -> std::pair<Precedence, opt::Option<Binding>> {
    return Binding::try_get_from(peek_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, opt::Option<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, opt::none});
}

auto Parser::parse_statement(bool require_semicolon) -> Result<ast::StatementHandle, Diagnostic> {
    // Not all decls are public so the condition needs to be rechecked
    if (current_token_.type == TokenType::PUBLIC) {
        switch (peek_token_.type) {
        case TokenType::IMPORT: return ast::ImportStatement::parse(*this);
        case TokenType::USING:  return ast::UsingStatement::parse(*this);
        default:                return ast::DeclStatement::parse(*this);
        }
    } else if (current_token_.is_decl_token()) {
        return ast::DeclStatement::parse(*this);
    }

    switch (current_token_.type) {
    case TokenType::LBRACE:     return ast::BlockStatement::parse(*this);
    case TokenType::BREAK:      return ast::BreakStatement::parse(*this);
    case TokenType::CONTINUE:   return ast::ContinueStatement::parse(*this);
    case TokenType::DEFER:      return ast::DeferStatement::parse(*this);
    case TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    case TokenType::IMPORT:     return ast::ImportStatement::parse(*this);
    case TokenType::RETURN:     return ast::ReturnStatement::parse(*this);
    case TokenType::TEST:       return ast::TestStatement::parse(*this);
    case TokenType::USING:      return ast::UsingStatement::parse(*this);
    default:                    return ast::ExpressionStatement::parse(*this, require_semicolon);
    }
}

auto Parser::parse_expression(Precedence precedence) -> Result<ast::ExpressionHandle, Diagnostic> {
    if (current_token_is(TokenType::END)) {
        return make_syntax_err(Error::END_OF_TOKEN_STREAM, current_token_);
    }

    const auto& prefix = get_prefix_fn_opt(current_token_.type);
    if (!prefix) {
        return make_syntax_err(fmt::format("No prefix parse function for {}({}) found",
                                           magic_enum::enum_name(current_token_.type),
                                           current_token_.slice),
                               Error::MISSING_PREFIX_PARSER,
                               current_token_);
    }
    auto lhs_expression = TRY((*prefix)(*this));

    while (!peek_token_is(TokenType::SEMICOLON) && precedence < get_peek_precedence().first) {
        const auto& infix = get_poll_infix_fn_opt(peek_token_.type);
        if (!infix) { break; }
        advance();
        lhs_expression = TRY((*infix)(*this, std::move(lhs_expression)));
    }

    return lhs_expression;
}

[[nodiscard]] auto Parser::parse_restricted_statement(Error error, bool require_semicolon)
    -> Result<ast::StatementHandle, Diagnostic> {
    auto clause = TRY(parse_statement(require_semicolon));

    // The clause can only be a jump, block, or expression statement
    if (!clause.any<ast::ExpressionStatement,
                    ast::BreakStatement,
                    ast::ContinueStatement,
                    ast::ReturnStatement,
                    ast::BlockStatement>()) {
        return make_syntax_err(error, ast_->location_of(*clause));
    }
    return clause;
}

[[nodiscard]] auto Parser::try_parse_restricted_alternate(Error error, bool require_semicolon)
    -> Result<opt::Option<ast::StatementHandle>, Diagnostic> {
    if (peek_token_is(TokenType::ELSE)) {
        // Advance twice to actually look at the statement's first token
        advance(2);
        return TRY(parse_restricted_statement(error, require_semicolon));
    }
    return opt::none;
}

namespace {

constexpr auto PREFIX_FNS = [] {
    fixed::EnumMap<TokenType, Parser::PrefixFn> fns;

    fns[TokenType::IDENT]            = ast::IdentifierExpression::parse;
    fns[TokenType::U8]               = ast::U8Expression::parse;
    fns[TokenType::F32]              = ast::F32Expression::parse;
    fns[TokenType::F64]              = ast::F64Expression::parse;
    fns[TokenType::BANG]             = ast::UnaryExpression::parse;
    fns[TokenType::NOT]              = ast::UnaryExpression::parse;
    fns[TokenType::MINUS]            = ast::UnaryExpression::parse;
    fns[TokenType::PLUS]             = ast::UnaryExpression::parse;
    fns[TokenType::STAR]             = ast::DereferenceExpression::parse;
    fns[TokenType::BW_AND]           = ast::ReferenceExpression::parse;
    fns[TokenType::AND_MUT]          = ast::ReferenceExpression::parse;
    fns[TokenType::CARET]            = ast::AddressOfExpression::parse;
    fns[TokenType::CARET_MUT]        = ast::AddressOfExpression::parse;
    fns[TokenType::DOT]              = ast::ImplicitAccessExpression::parse;
    fns[TokenType::BOOLEAN_TRUE]     = ast::BoolExpression::parse;
    fns[TokenType::BOOLEAN_FALSE]    = ast::BoolExpression::parse;
    fns[TokenType::UNDEFINED]        = ast::UndefinedExpression::parse;
    fns[TokenType::LBRACE]           = ast::VoidExpression::parse;
    fns[TokenType::STRING]           = ast::StringExpression::parse;
    fns[TokenType::MULTILINE_STRING] = ast::StringExpression::parse;
    fns[TokenType::LPAREN]           = ast::GroupedExpression::parse;
    fns[TokenType::IF]               = ast::IfExpression::parse;
    fns[TokenType::FUNCTION]         = ast::FunctionExpression::parse;
    fns[TokenType::STRUCT]           = ast::StructExpression::parse;
    fns[TokenType::UNION]            = ast::UnionExpression::parse;
    fns[TokenType::ENUM]             = ast::EnumExpression::parse;
    fns[TokenType::MATCH]            = ast::MatchExpression::parse;
    fns[TokenType::LBRACKET]         = ast::ArrayExpression::parse;
    fns[TokenType::FOR]              = ast::ForLoopExpression::parse;
    fns[TokenType::WHILE]            = ast::WhileLoopExpression::parse;
    fns[TokenType::DO]               = ast::DoWhileLoopExpression::parse;
    fns[TokenType::LOOP]             = ast::InfiniteLoopExpression::parse;

    for (const auto tt : enum_range<TokenType::INT_2, TokenType::UZINT_16>()) {
        using token_type::IntegerCategory;
        switch (token_type::to_int_category(tt)) {
        case IntegerCategory::SIGNED_BASE:   fns[tt] = ast::I32Expression::parse; break;
        case IntegerCategory::SIGNED_WIDE:   fns[tt] = ast::I64Expression::parse; break;
        case IntegerCategory::SIGNED_SIZE:   fns[tt] = ast::ISizeExpression::parse; break;
        case IntegerCategory::UNSIGNED_BASE: fns[tt] = ast::U32Expression::parse; break;
        case IntegerCategory::UNSIGNED_WIDE: fns[tt] = ast::U64Expression::parse; break;
        case IntegerCategory::UNSIGNED_SIZE: fns[tt] = ast::USizeExpression::parse; break;
        }
    }

    for (const auto tt : ALL_PRIMITIVES) { fns[tt] = ast::IdentifierExpression::parse; }
    for (const auto tt : builtins::ALL_TOKEN_TYPES) { fns[tt] = ast::IdentifierExpression::parse; }

    return fns;
}();

constexpr auto INFIX_FNS = [] {
    fixed::EnumMap<TokenType, Parser::InfixFn> fns;

    fns[TokenType::PLUS]           = ast::BinaryExpression::parse;
    fns[TokenType::MINUS]          = ast::BinaryExpression::parse;
    fns[TokenType::STAR]           = ast::BinaryExpression::parse;
    fns[TokenType::SLASH]          = ast::BinaryExpression::parse;
    fns[TokenType::PERCENT]        = ast::BinaryExpression::parse;
    fns[TokenType::LT]             = ast::BinaryExpression::parse;
    fns[TokenType::LT_EQ]          = ast::BinaryExpression::parse;
    fns[TokenType::GT]             = ast::BinaryExpression::parse;
    fns[TokenType::GT_EQ]          = ast::BinaryExpression::parse;
    fns[TokenType::EQ]             = ast::BinaryExpression::parse;
    fns[TokenType::NEQ]            = ast::BinaryExpression::parse;
    fns[TokenType::BOOLEAN_AND]    = ast::BinaryExpression::parse;
    fns[TokenType::BOOLEAN_OR]     = ast::BinaryExpression::parse;
    fns[TokenType::BW_AND]         = ast::BinaryExpression::parse;
    fns[TokenType::BW_OR]          = ast::BinaryExpression::parse;
    fns[TokenType::CARET]          = ast::BinaryExpression::parse;
    fns[TokenType::SHR]            = ast::BinaryExpression::parse;
    fns[TokenType::SHL]            = ast::BinaryExpression::parse;
    fns[TokenType::DOT]            = ast::DotExpression::parse;
    fns[TokenType::DOT_DOT]        = ast::RangeExpression::parse;
    fns[TokenType::DOT_DOT_EQ]     = ast::RangeExpression::parse;
    fns[TokenType::LPAREN]         = ast::CallExpression::parse;
    fns[TokenType::LBRACKET]       = ast::IndexExpression::parse;
    fns[TokenType::LBRACE]         = ast::InitializerExpression::parse;
    fns[TokenType::ASSIGN]         = ast::AssignmentExpression::parse;
    fns[TokenType::PLUS_ASSIGN]    = ast::AssignmentExpression::parse;
    fns[TokenType::MINUS_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::STAR_ASSIGN]    = ast::AssignmentExpression::parse;
    fns[TokenType::SLASH_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::PERCENT_ASSIGN] = ast::AssignmentExpression::parse;
    fns[TokenType::BW_AND_ASSIGN]  = ast::AssignmentExpression::parse;
    fns[TokenType::BW_OR_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::SHL_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::SHR_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::NOT_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::XOR_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::COLON_COLON]    = ast::ModuleAccessExpression::parse;
    fns[TokenType::COLON]          = ast::LabelExpression::parse;

    return fns;
}();

} // namespace

auto Parser::get_prefix_fn_opt(TokenType tt) noexcept -> opt::Option<PrefixFn> {
    return PREFIX_FNS.get_opt(tt);
}

auto Parser::get_poll_infix_fn_opt(TokenType tt) noexcept -> opt::Option<InfixFn> {
    return INFIX_FNS.get_opt(tt);
}

auto Parser::get_location_of(ast::NodeID id) -> SourceLocation { return ast_->location_of(id); }

auto Parser::get_location_of(ast::ExplicitTypeID id) -> SourceLocation {
    return ast_->location_of(id);
}

} // namespace porpoise::syntax
