#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace conch::ast {

class IdentifierExpression;
class StringExpression;

class ImportStatement : public StmtBase<ImportStatement> {
  public:
    static constexpr auto KIND = NodeKind::IMPORT_STATEMENT;

    using ModuleImport = Box<IdentifierExpression>;
    using UserImport   = Box<StringExpression>;

  public:
    explicit ImportStatement(const Token&                           start_token,
                             std::variant<ModuleImport, UserImport> imported,
                             Optional<Box<IdentifierExpression>>    alias) noexcept;
    ~ImportStatement() override;

    MAKE_AST_COPY_MOVE(ImportStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    MAKE_VARIANT_UNPACKER(module_import, IdentifierExpression, ModuleImport, imported_, *std::get)
    MAKE_VARIANT_UNPACKER(user_import, StringExpression, UserImport, imported_, *std::get)

    [[nodiscard]] auto has_alias() const noexcept -> bool { return alias_.has_value(); }
    [[nodiscard]] auto get_alias() const noexcept -> Optional<const IdentifierExpression&> {
        return alias_ ? Optional<const IdentifierExpression&>{**alias_} : nullopt;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::variant<ModuleImport, UserImport> imported_;
    Optional<Box<IdentifierExpression>>    alias_;
};

} // namespace conch::ast
