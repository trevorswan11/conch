#include "ast/statement.hh"

#include <bit>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include "ast/ast.hh"
#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/type.hh"
#include "syntax/error.hh"
#include "syntax/parser.hh"
#include "syntax/token_type.hh"

#include "fixed/enum_map.hh"
#include "option.hh"
#include "result.hh"

namespace ghoti::ast {

auto BlockStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    Statements statements;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        statements.emplace_back(TRY(parser.parse_statement()));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return parser.add_stmt<BlockStatement>(start_token, std::move(statements));
}

auto BreakStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Labels are optional
    opt::Option<IdentifierHandle> label;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        label.emplace(TRY(IdentifierExpression::parse(parser)));
    }

    // Values can be present but must be associated with a label
    opt::Option<ExpressionHandle> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value.emplace(TRY(parser.parse_expression()));
    }

    if (value && !label) {
        return make_syntax_err("Valued break statements must be labeled",
                               syntax::Error::VALUED_BREAK_MISSING_LABEL,
                               start_token);
    }
    TRY(parser.expect_semicolon());
    return parser.add_stmt<BreakStatement>(start_token, label, value);
}

auto ContinueStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Labels are optional
    opt::Option<IdentifierHandle> label;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        label.emplace(TRY(IdentifierExpression::parse(parser)));
    }

    // Values can never be present in a continue
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err("Continue statements may only contain labels",
                               syntax::Error::VALUED_CONTINUE,
                               start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt<ContinueStatement>(start_token, label);
}

namespace {

using ModifierMapping          = std::pair<syntax::TokenType, DeclModifiers>;
constexpr auto LEGAL_MODIFIERS = [] {
    using TokenType = syntax::TokenType;
    fixed::EnumMap<TokenType, opt::Option<DeclModifiers>> modifiers{opt::none};
    modifiers[TokenType::VAR]       = DeclModifiers::VARIABLE;
    modifiers[TokenType::CONSTANT]  = DeclModifiers::CONSTANT;
    modifiers[TokenType::CONSTEXPR] = DeclModifiers::CONSTEXPR;
    modifiers[TokenType::PUBLIC]    = DeclModifiers::PUBLIC;
    modifiers[TokenType::EXTERN]    = DeclModifiers::EXTERN;
    modifiers[TokenType::EXPORT]    = DeclModifiers::EXPORT;
    return modifiers;
}();

[[nodiscard]] constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept
    -> opt::Option<std::string> {
    const auto mut_count = std::popcount(
        std::to_underlying(modifiers & (DeclModifiers::VARIABLE | DeclModifiers::CONSTANT |
                                        DeclModifiers::CONSTEXPR)));
    if (mut_count != 1) {
        return fmt::format("Exactly one mutability modifier may be used; found {}", mut_count);
    }

    const auto valid_constexpr =
        std::popcount(std::to_underlying(modifiers &
                                         (DeclModifiers::EXTERN | DeclModifiers::CONSTEXPR))) <= 1;
    if (!valid_constexpr) { return "Extern values cannot be known at compile time"; }

    const auto abi_count = std::popcount(
        std::to_underlying(modifiers & (DeclModifiers::EXTERN | DeclModifiers::EXPORT)));
    if (abi_count > 1) {
        return fmt::format("At most one ABI-related modifier may be used; found {}", abi_count);
    }
    return opt::none;
}

} // namespace

auto DeclStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    auto       modifiers   = LEGAL_MODIFIERS[start_token.type].value();

    opt::Option<DeclModifiers> current_modifier;
    while ((current_modifier = LEGAL_MODIFIERS[parser.get_peek_token().type])) {
        parser.advance();
        if (modifiers_has(modifiers, *current_modifier)) {
            return make_syntax_err("Declaration modifiers may only be used once in any order",
                                   syntax::Error::DUPLICATE_DECL_MODIFIER,
                                   parser.get_current_token());
        }
        modifiers |= *current_modifier;
    }

    if (auto msg = validate_modifiers(modifiers)) {
        return make_syntax_err(
            std::move(msg).value(), syntax::Error::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    const IdentifierHandle decl_name          = TRY(IdentifierExpression::parse(parser));
    const auto [decl_type, value_initialized] = TRY(ExplicitType::parse_opt_init(parser));

    opt::Option<ExpressionHandle> decl_value;
    if (value_initialized) {
        decl_value.emplace(TRY(parser.parse_expression()));

        // If there is a value, then there cannot be an extern due to a contradiction
        if (modifiers_has(modifiers, DeclModifiers::EXTERN)) {
            return make_syntax_err("Extern declarations may not be value-initialized",
                                   syntax::Error::EXTERN_VALUE_INITIALIZED,
                                   start_token);
        }
    } else if ((modifiers_has(modifiers, DeclModifiers::CONSTANT) &&
                !modifiers_has(modifiers, DeclModifiers::EXTERN)) ||
               modifiers_has(modifiers, DeclModifiers::CONSTEXPR)) {
        // Constant decls must be declared with a value unless they are extern
        return make_syntax_err("Constant non-extern declarations must have an associated value",
                               syntax::Error::CONST_DECL_MISSING_VALUE,
                               start_token);
    }

    TRY(parser.expect_semicolon());
    return parser.add_stmt<DeclStatement>(start_token, decl_name, decl_type, decl_value, modifiers);
}

auto DeferStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err("Defer statements require an statement to defer",
                               syntax::Error::DEFER_MISSING_DEFERREE,
                               start_token);
    }
    parser.advance();
    const auto stmt = TRY(parser.parse_statement());

    // The statement has different restrictions from expression alternates
    if (!stmt.any<ExpressionStatement, DiscardStatement, BlockStatement>()) {
        return make_syntax_err("Deferred statements must be expressions, discards, or blocks",
                               syntax::Error::ILLEGAL_DEFERRED_STATEMENT,
                               parser.get_location_of(*stmt));
    }
    return parser.add_stmt<DeferStatement>(start_token, stmt);
}

auto DiscardStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err("Discarded statements must have a statement to discard",
                               syntax::Error::DISCARD_MISSING_DISCARDEE,
                               parser.get_current_token());
    }

    parser.advance();
    const auto expr = TRY(parser.parse_expression());

    TRY(parser.expect_semicolon());
    return parser.add_stmt<DiscardStatement>(start_token, expr);
}

auto ExpressionStatement::parse(syntax::Parser& parser, syntax::SemicolonBehavior behavior)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    const auto expr        = TRY(parser.parse_expression());

    using Behavior           = syntax::SemicolonBehavior;
    const bool has_semicolon = parser.current_token_is(syntax::TokenType::SEMICOLON);
    const bool at_block_end  = parser.current_token_is(syntax::TokenType::RBRACE);

    const auto check_illegal_semicolon = [&] -> opt::Option<Err<syntax::Diagnostic>> {
        const auto& semicolon = parser.advance();
        if (behavior == Behavior::DISALLOW) {
            return syntax::make_syntax_err("Semicolon is not allowed in this context",
                                           syntax::Error::UNEXPECTED_TOKEN,
                                           semicolon);
        }
        return opt::none;
    };

    // RBRACE would mean we're at the end of a block and a semicolon is never required
    if (at_block_end) {
        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
            if (auto err = check_illegal_semicolon()) { return std::move(*err); }
        }
    } else if (!has_semicolon) {
        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
            if (auto err = check_illegal_semicolon()) { return std::move(*err); }
        } else if (behavior == Behavior::REQUIRE) {
            TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
        }
    }
    return parser.add_stmt<ExpressionStatement>(start_token, expr);
}

namespace {

[[nodiscard]] auto parse_import_payload(syntax::Parser& parser)
    -> Result<ImportPayloadHandle, syntax::Diagnostic> {
    if (parser.peek_token_is(syntax::TokenType::IDENT)) {
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        return TRY(IdentifierExpression::parse(parser));
    } else if (parser.peek_token_is(syntax::TokenType::STRING)) {
        TRY(parser.expect_peek(syntax::TokenType::STRING));
        const StringHandle string = TRY(StringExpression::parse(parser));

        if (parser.get_node<StringExpression>(*string).value.empty()) {
            return make_syntax_err("File import names cannot be empty",
                                   syntax::Error::EMPTY_FILE_IMPORT,
                                   parser.get_location_of(*string));
        }
        return string;
    }

    return make_syntax_err("Imported payloads may only be filename strings or module identifiers",
                           syntax::Error::ILLEGAL_IMPORT_TYPE,
                           parser.get_peek_token());
}

} // namespace

auto ImportStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    // A start token of public is guaranteed to be followed by an import
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PUBLIC)) { parser.advance(); }
    auto imported_core = TRY(parse_import_payload(parser));

    opt::Option<IdentifierHandle> imported_alias;
    if (parser.peek_token_is(syntax::TokenType::AS)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));

        imported_alias.emplace(TRY(IdentifierExpression::parse(parser)));
    } else if (imported_core->get_kind() == NodeKind::STRING_EXPRESSION) {
        return make_syntax_err("All file imports must be aliased to an identifier",
                               syntax::Error::FILE_IMPORT_MISSING_ALIAS,
                               start_token);
    }

    TRY(parser.expect_semicolon());
    return parser.add_stmt<ImportStatement>(start_token, imported_core, imported_alias);
}

auto ImportStatement::get_name(const AST& tree) const noexcept
    -> std::pair<ast::IdentifierHandle, std::string_view> {
    if (alias) {
        const auto  handle = *alias;
        const auto& ident  = tree.get_as<ast::IdentifierExpression>(handle);
        return {handle, ident.name};
    }

    // This must be an ident as all strings have an alias
    const auto& ident = tree.get_as<ast::IdentifierExpression>(payload);
    return {payload, ident.name};
}

auto ReturnStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<ExpressionHandle> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value.emplace(TRY(parser.parse_expression()));
    }

    TRY(parser.expect_semicolon());
    return parser.add_stmt<ReturnStatement>(start_token, value);
}

auto TestStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<StringHandle> description;
    if (parser.peek_token_is(syntax::TokenType::STRING)) {
        parser.advance();
        description.emplace(TRY(StringExpression::parse(parser)));

        // Empty strings aren't supported since one should just use no description
        if (parser.get_node<StringExpression>(**description).value.empty()) {
            return make_syntax_err("Test descriptions may not be empty when present",
                                   syntax::Error::EMPTY_TEST_DESCRIPTION,
                                   parser.get_location_of(**description));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    const BlockHandle block = TRY(BlockStatement::parse(parser));
    return parser.add_stmt<TestStatement>(start_token, description, block);
}

auto UsingStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    // A start token of public is guaranteed to be followed by an import
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PUBLIC)) { parser.advance(); }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    const IdentifierHandle alias = TRY(IdentifierExpression::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    const auto type = TRY(ExplicitType::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt<UsingStatement>(start_token, alias, type);
}

} // namespace ghoti::ast
