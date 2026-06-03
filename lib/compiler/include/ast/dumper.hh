#pragma once

#include <ostream>
#include <variant>

#include <fmt/ostream.h>

#include "ast/ast.hh"
#include "ast/format.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/traits.hh"
#include "ast/visitor.hh"

#include "assert.hh"
#include "indent.hh"

namespace ghoti::ast {

class ASTDumper {
  public:
    explicit ASTDumper(const AST& ast, std::ostream& out) : out_{out}, ast_{ast} {}

    template <traits::IndexableNodeID ID> auto dump(ID id) -> void {
        ASSERT(id.is_valid(), "Attempt to dump invalid handle");
        std::visit([&](const auto& data) { this->visit(id, data); }, ast_[id]);
    }

    auto dump(ExplicitTypeID id) -> void {
        ASSERT(id.is_valid(), "Attempt to dump invalid handle");
        fmt::println(out_, "ExplicitType (modifier: {})", id.get_modifier());
        const Indent::Guard g{indent_, true};
        std::visit([&](const auto& data) { visit(id, data); }, ast_[id]);
    }

  private:
    AST_VISITOR_DEF_GEN()

    template <typename T, typename Func> void dump_container(const T& container, Func&& func) {
        for (auto it = container.begin(); it != container.end(); ++it) {
            Indent::Guard g{indent_, std::next(it) == container.end()};
            std::forward<Func>(func)(*it);
        }
    }

    template <typename T> void dump_node_list(const T& list) {
        dump_container(list, [this](const auto& node_handle) {
            fmt::print(out_, "{}", indent_.current_branch());
            dump(*node_handle);
        });
    }

    template <> void dump_node_list<Members>(const Members& list) {
        dump_container(list, [this](const MemberHandle& member_handle) {
            fmt::print(out_, "{}", indent_.current_branch());
            dump(*member_handle);
        });
    }

  private:
    std::ostream& out_;
    const AST&    ast_;
    Indent        indent_;
};

} // namespace ghoti::ast
