#include "ast/statements/import.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

ImportStatement::ImportStatement(const Token&                           start_token,
                                 std::variant<ModuleImport, UserImport> imported,
                                 Optional<Box<IdentifierExpression>>    alias) noexcept
    : Statement{start_token}, imported_{std::move(imported)}, alias_{std::move(alias)} {}

ImportStatement::~ImportStatement() = default;

auto ImportStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImportStatement::parse(Parser& parser) -> Expected<Box<ImportStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::variant<ModuleImport, UserImport> imported;
    if (parser.peek_token_is(TokenType::IDENT)) {
        TRY(parser.expect_peek(TokenType::IDENT));
        auto identifier = TRY(IdentifierExpression::parse(parser));
        imported        = std::move(identifier);
    } else if (parser.peek_token_is(TokenType::STRING)) {
        TRY(parser.expect_peek(TokenType::STRING));
        auto str = TRY(IdentifierExpression::parse(parser));
        imported = std::move(str);
    } else {
        return make_parser_unexpected(ParserError::EMPTY_IMPL_BLOCK, parser.peek_token());
    }

    Optional<Box<IdentifierExpression>> imported_alias;
    if (parser.peek_token_is(TokenType::AS)) {
        parser.advance();
        TRY(parser.expect_peek(TokenType::IDENT));

        imported_alias = TRY(IdentifierExpression::parse(parser));
    } else if (std::holds_alternative<Box<StringExpression>>(imported)) {
        return make_parser_unexpected(ParserError::USER_IMPORT_MISSING_ALIAS, start_token);
    }

    return make_box<ImportStatement>(start_token, std::move(imported), std::move(imported_alias));
}

} // namespace conch::ast
