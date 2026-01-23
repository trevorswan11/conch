#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement : public Statement {
  public:
    explicit BlockStatement(const Token&                            start_token,
                            std::vector<std::unique_ptr<Statement>> statements) noexcept
        : Statement{start_token}, statements_{std::move(statements)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<BlockStatement>, ParserDiagnostic>;

  public:
    using iterator       = typename std::vector<std::unique_ptr<Statement>>::iterator;
    using const_iterator = typename std::vector<std::unique_ptr<Statement>>::const_iterator;

    [[nodiscard]] auto begin() noexcept -> iterator { return statements_.begin(); }
    [[nodiscard]] auto end() noexcept -> iterator { return statements_.end(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return statements_.begin(); }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return statements_.end(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t { return statements_.size(); }
    [[nodiscard]] auto empty() const noexcept -> bool { return statements_.empty(); }

  private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

} // namespace ast
