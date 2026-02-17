#include "ast/statements/import.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

ImportStatement::ImportStatement(const Token&                           start_token,
                                 std::variant<ModuleImport, UserImport> imported,
                                 Optional<Box<IdentifierExpression>>    alias) noexcept
    : StmtBase{start_token}, imported_{std::move(imported)}, alias_{std::move(alias)} {}

ImportStatement::~ImportStatement() = default;

auto ImportStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImportStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::variant<ModuleImport, UserImport> imported;
    if (parser.peek_token_is(TokenType::IDENT)) {
        TRY(parser.expect_peek(TokenType::IDENT));
        imported = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    } else if (parser.peek_token_is(TokenType::STRING)) {
        TRY(parser.expect_peek(TokenType::STRING));
        imported = downcast<StringExpression>(TRY(StringExpression::parse(parser)));
    } else {
        return make_parser_unexpected(ParserError::ILLEGAL_IMPORT, parser.peek_token());
    }

    Optional<Box<IdentifierExpression>> imported_alias;
    if (parser.peek_token_is(TokenType::AS)) {
        parser.advance();
        TRY(parser.expect_peek(TokenType::IDENT));

        imported_alias = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    } else if (std::holds_alternative<Box<StringExpression>>(imported)) {
        return make_parser_unexpected(ParserError::USER_IMPORT_MISSING_ALIAS, start_token);
    }

    return make_box<ImportStatement>(start_token, std::move(imported), std::move(imported_alias));
}

auto ImportStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted         = as<ImportStatement>(other);
    const auto& other_imported = casted.imported_;
    const auto  variant_eq     = std::visit(Overloaded{
                                           [&other_imported](const ModuleImport& v) {
                                               return *v == *std::get<ModuleImport>(other_imported);
                                           },
                                           [&other_imported](const UserImport& v) {
                                               return *v == *std::get<UserImport>(other_imported);
                                           },
                                       },
                                       imported_);
    return variant_eq && optional::unsafe_eq<IdentifierExpression>(alias_, casted.alias_);
}

} // namespace conch::ast
