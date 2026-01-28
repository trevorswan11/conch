#pragma once

#include <memory>
#include <variant>

#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class StringExpression;

class ImportStatement : public Statement {
  public:
    explicit ImportStatement(const Token&                                    start_token,
                             std::variant<std::unique_ptr<IdentifierExpression>,
                                          std::unique_ptr<StringExpression>> import,
                             Optional<std::unique_ptr<IdentifierExpression>> alias) noexcept;
    ~ImportStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ImportStatement>, ParserDiagnostic>;

    using ImportTarget = std::variant<const IdentifierExpression*, const StringExpression*>;
    [[nodiscard]] auto import_target() const noexcept -> ImportTarget {
        return std::visit([](const auto& ptr) -> ImportTarget { return ptr.get(); }, import_);
    }

    [[nodiscard]] auto alias() const noexcept -> Optional<const IdentifierExpression&> {
        return alias_ ? Optional<const IdentifierExpression&>{**alias_} : nullopt;
    }

  private:
    std::variant<std::unique_ptr<IdentifierExpression>, std::unique_ptr<StringExpression>> import_;
    Optional<std::unique_ptr<IdentifierExpression>>                                        alias_;
};

} // namespace ast
