#pragma once

#include <memory>
#include <optional>
#include <span>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;

struct StructMember {
    explicit StructMember(std::unique_ptr<IdentifierExpression> n,
                          std::unique_ptr<TypeExpression>       t) noexcept;
    explicit StructMember(std::unique_ptr<IdentifierExpression>      n,
                          std::unique_ptr<TypeExpression>            t,
                          std::optional<std::unique_ptr<Expression>> d) noexcept;
    ~StructMember();

    StructMember(const StructMember&)                        = delete;
    auto operator=(const StructMember&) -> StructMember&     = delete;
    StructMember(StructMember&&) noexcept                    = default;
    auto operator=(StructMember&&) noexcept -> StructMember& = default;

    std::unique_ptr<IdentifierExpression>      name;
    std::unique_ptr<TypeExpression>            type;
    std::optional<std::unique_ptr<Expression>> default_value;
};

class StructExpression : public Expression {
  public:
    explicit StructExpression(const Token&                          start_token,
                              std::unique_ptr<IdentifierExpression> name,
                              std::vector<StructMember>             members) noexcept;
    ~StructExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<StructExpression>, ParserDiagnostic>;

    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto members() const noexcept -> std::span<const StructMember> {
        return members_;
    }

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<StructMember>             members_;
};

} // namespace ast
