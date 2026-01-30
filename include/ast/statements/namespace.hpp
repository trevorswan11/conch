#pragma once

#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class ScopeResolutionExpression;
class BlockStatement;

class NamespaceStatement : public Statement {
  public:
    using Single = Box<IdentifierExpression>;
    using Nested = Box<ScopeResolutionExpression>;

  public:
    explicit NamespaceStatement(const Token&                 start_token,
                                std::variant<Single, Nested> nspace,
                                Box<BlockStatement>          block) noexcept;
    ~NamespaceStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<NamespaceStatement>, ParserDiagnostic>;

    // UB if the import is not a single/shallow namespace.
    [[nodiscard]] auto get_single_namespace() const noexcept -> const IdentifierExpression& {
        try {
            return *std::get<Single>(namespace_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_single_namespace() const noexcept -> bool {
        return std::holds_alternative<Single>(namespace_);
    }

    // UB if the import is not a nested namespace.
    [[nodiscard]] auto get_nested_namespace() const noexcept -> const ScopeResolutionExpression& {
        try {
            return *std::get<Nested>(namespace_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_nested_namespace() const noexcept -> bool {
        return std::holds_alternative<Nested>(namespace_);
    }

  private:
    std::variant<Single, Nested> namespace_;
    Box<BlockStatement>          block_;
};

} // namespace conch::ast
