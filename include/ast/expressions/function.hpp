#pragma once

#include <memory>
#include <span>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;
class BlockStatement;

class FunctionParameter {
  public:
    explicit FunctionParameter(bool                                  reference,
                               std::unique_ptr<IdentifierExpression> name,
                               std::unique_ptr<TypeExpression>       type) noexcept;
    explicit FunctionParameter(bool                                  reference,
                               std::unique_ptr<IdentifierExpression> name,
                               std::unique_ptr<TypeExpression>       type,
                               Optional<std::unique_ptr<Expression>> default_value) noexcept;
    ~FunctionParameter();

    FunctionParameter(const FunctionParameter&)                        = delete;
    auto operator=(const FunctionParameter&) -> FunctionParameter&     = delete;
    FunctionParameter(FunctionParameter&&) noexcept                    = default;
    auto operator=(FunctionParameter&&) noexcept -> FunctionParameter& = default;

    [[nodiscard]] auto reference() const noexcept -> bool { return reference_; }
    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto type() const noexcept -> const TypeExpression& { return *type_; }
    [[nodiscard]] auto body() const noexcept -> Optional<const Expression&> {
        return default_value_ ? Optional<const Expression&>{**default_value_} : nullopt;
    }

  private:
    bool                                  reference_;
    std::unique_ptr<IdentifierExpression> name_;
    std::unique_ptr<TypeExpression>       type_;
    Optional<std::unique_ptr<Expression>> default_value_;
};

class FunctionExpression : public Expression {
  public:
    explicit FunctionExpression(const Token&                              start_token,
                                std::vector<FunctionParameter>            parameters,
                                std::unique_ptr<TypeExpression>           return_type,
                                Optional<std::unique_ptr<BlockStatement>> body) noexcept;
    ~FunctionExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<FunctionExpression>, ParserDiagnostic>;

    [[nodiscard]] auto parameters() const noexcept -> std::span<const FunctionParameter> {
        return parameters_;
    }

    [[nodiscard]] auto return_type() const noexcept -> const TypeExpression& {
        return *return_type_;
    }

    [[nodiscard]] auto body() const noexcept -> Optional<const BlockStatement&> {
        return body_ ? Optional<const BlockStatement&>{**body_} : nullopt;
    }

  private:
    std::vector<FunctionParameter>            parameters_;
    std::unique_ptr<TypeExpression>           return_type_;
    Optional<std::unique_ptr<BlockStatement>> body_;
};

} // namespace ast
