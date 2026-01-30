#pragma once

#include <utility>
#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class StringExpression;

class ImportStatement : public Statement {
  public:
    using ModuleImport = Box<IdentifierExpression>;
    using UserImport   = Box<StringExpression>;

  public:
    explicit ImportStatement(const Token&                           start_token,
                             std::variant<ModuleImport, UserImport> imported,
                             Optional<Box<IdentifierExpression>>    alias) noexcept;
    ~ImportStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<ImportStatement>, ParserDiagnostic>;

    // UB if the import is not a module import.
    [[nodiscard]] auto get_module_import() const noexcept -> const IdentifierExpression& {
        try {
            return *std::get<ModuleImport>(imported_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_module_import() const noexcept -> bool {
        return std::holds_alternative<ModuleImport>(imported_);
    }

    // UB if the import is not a user import.
    [[nodiscard]] auto get_user_import() const noexcept -> const StringExpression& {
        try {
            return *std::get<UserImport>(imported_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_user_import() const noexcept -> bool {
        return std::holds_alternative<UserImport>(imported_);
    }

    [[nodiscard]] auto has_alias() const noexcept -> bool { return alias_.has_value(); }
    [[nodiscard]] auto get_alias() const noexcept -> Optional<const IdentifierExpression&> {
        return alias_ ? Optional<const IdentifierExpression&>{**alias_} : nullopt;
    }

  private:
    std::variant<ModuleImport, UserImport> imported_;
    Optional<Box<IdentifierExpression>>    alias_;
};

} // namespace conch::ast
