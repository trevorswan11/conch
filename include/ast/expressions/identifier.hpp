#pragma once

#include <string>
#include <string_view>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression : public Expression {
  public:
    explicit IdentifierExpression(const Token& start_token, std::string_view name) noexcept
        : Expression{start_token, NodeKind::IDENTIFIER_EXPRESSION}, name_{name} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<IdentifierExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]] auto materialize() const -> std::string { return std::string{name_}; }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<IdentifierExpression>(other);
        return name_ == casted.name_;
    }

  private:
    std::string_view name_;
};

} // namespace conch::ast
