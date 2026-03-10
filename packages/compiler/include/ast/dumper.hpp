#pragma once

#include <ostream>

#include <fmt/ostream.h>

#include "ast/visitor.hpp"

#include "indent.hpp"

namespace conch::ast {

class ExplicitType;

class ASTDumper : public Visitor {
  public:
    explicit ASTDumper(std::ostream& out) : out_{out} {}

    AST_VISITOR_OVERRIDES()

  private:
    auto dump_explicit_type(const ExplicitType& type, bool print_branch) -> void;

    template <typename T, typename Func> void dump_container(const T& container, Func&& func) {
        for (auto it = container.begin(); it != container.end(); ++it) {
            Indent::Guard g{indent_, std::next(it) == container.end()};
            std::forward<Func>(func)(*it);
        }
    }

    template <typename T> void dump_node_list(const T& list) {
        dump_container(list, [this](const auto& node) {
            fmt::print(out_, "{}", indent_.current_branch());
            node->accept(*this);
        });
    }

  private:
    std::ostream& out_;
    Indent        indent_;
};

} // namespace conch::ast
