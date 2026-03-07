#pragma once

#include <span>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;
class BlockStatement;

class FunctionParameter {
  public:
    explicit FunctionParameter(Box<IdentifierExpression> name, Box<TypeExpression> type) noexcept;
    ~FunctionParameter();

    FunctionParameter(const FunctionParameter&)                        = delete;
    auto operator=(const FunctionParameter&) -> FunctionParameter&     = delete;
    FunctionParameter(FunctionParameter&&) noexcept                    = default;
    auto operator=(FunctionParameter&&) noexcept -> FunctionParameter& = default;

    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto get_type() const noexcept -> const TypeExpression& { return *type_; }

  private:
    Box<IdentifierExpression> name_;
    Box<TypeExpression>       type_;

    friend class FunctionExpression;
};

class SelfParameter {
  public:
    explicit SelfParameter(Optional<TypeModifier>    modifier,
                           Box<IdentifierExpression> name) noexcept;
    ~SelfParameter();

    SelfParameter(const SelfParameter&)                        = delete;
    auto operator=(const SelfParameter&) -> SelfParameter&     = delete;
    SelfParameter(SelfParameter&&) noexcept                    = default;
    auto operator=(SelfParameter&&) noexcept -> SelfParameter& = default;

    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto has_modifier() const noexcept -> bool { return modifier_.has_value(); }
    [[nodiscard]] auto get_modifier() const noexcept -> Optional<TypeModifier> { return modifier_; }

  private:
    Optional<TypeModifier>    modifier_;
    Box<IdentifierExpression> name_;

    friend class FunctionExpression;
};

class FunctionExpression : public ExprBase<FunctionExpression> {
  public:
    static constexpr auto KIND = NodeKind::FUNCTION_EXPRESSION;

  public:
    explicit FunctionExpression(const Token&                   start_token,
                                bool                           mut,
                                Optional<SelfParameter>        self,
                                std::vector<FunctionParameter> parameters,
                                Box<TypeExpression>            return_type,
                                Optional<Box<BlockStatement>>  body) noexcept;
    ~FunctionExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto is_mutable() const noexcept -> bool { return mutable_; }
    [[nodiscard]] auto get_parameters() const noexcept -> std::span<const FunctionParameter> {
        return parameters_;
    }

    [[nodiscard]] auto get_return_type() const noexcept -> const TypeExpression& {
        return *return_type_;
    }

    [[nodiscard]] auto has_body() const noexcept -> bool { return body_.has_value(); }
    [[nodiscard]] auto get_body() const noexcept -> Optional<const BlockStatement&> {
        return body_ ? Optional<const BlockStatement&>{**body_} : nullopt;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    bool                           mutable_;
    Optional<SelfParameter>        self_;
    std::vector<FunctionParameter> parameters_;
    Box<TypeExpression>            return_type_;
    Optional<Box<BlockStatement>>  body_;
};

} // namespace conch::ast
