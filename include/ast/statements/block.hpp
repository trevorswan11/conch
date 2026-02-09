#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BlockStatement : public StmtBase<BlockStatement> {
  public:
    static constexpr auto KIND = NodeKind::BLOCK_STATEMENT;

  public:
    using iterator       = typename std::vector<Box<Statement>>::iterator;
    using const_iterator = typename std::vector<Box<Statement>>::const_iterator;

  public:
    explicit BlockStatement(const Token&                start_token,
                            std::vector<Box<Statement>> statements) noexcept
        : StmtBase{start_token}, statements_{std::move(statements)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    [[nodiscard]] auto begin() noexcept -> iterator { return statements_.begin(); }
    [[nodiscard]] auto end() noexcept -> iterator { return statements_.end(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return statements_.begin(); }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return statements_.end(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t { return statements_.size(); }
    [[nodiscard]] auto empty() const noexcept -> bool { return statements_.empty(); }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<BlockStatement>(other);
        return std::ranges::equal(
            statements_, casted.statements_, [](const auto& a, const auto& b) { return *a == *b; });
    }

  private:
    std::vector<Box<Statement>> statements_;
};

} // namespace conch::ast
