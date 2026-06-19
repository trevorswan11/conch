#pragma once

#include <ostream>

#include <fmt/ostream.h>
#include <stdx/assert.hh>
#include <stdx/variant.hh>

#include "ast/ast.hh"
#include "ast/format.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/type.hh"

#include <indent.hh>

namespace ghoti::ast {

class ASTDumper {
  public:
    explicit ASTDumper(const AST& ast, std::ostream& out) : out_{out}, ast_{ast} {}

    template <IndexableNodeID ID> auto dump(ID id) -> void {
        ast_[id].visit([&](const auto& data) -> void { this->visit(id, data); });
    }

    auto dump(ExplicitTypeID id) -> void {
        fmt::println(out_, "ExplicitType (modifier: {})", id.get_modifier());
        const Indent::Guard g{indent_, true};
        ast_[id].visit([&](const auto& data) -> void { visit(id, data); });
    }

  private:
    auto visit(NodeID, const ArrayExpression&) -> void;
    auto visit(NodeID, const CallExpression&) -> void;
    auto visit(NodeID, const DoWhileLoopExpression&) -> void;
    auto visit(NodeID, const EnumExpression&) -> void;
    auto visit(NodeID, const ForLoopExpression&) -> void;
    auto visit(NodeID, const FunctionExpression&) -> void;
    auto visit(NodeID, const IdentifierExpression&) -> void;
    auto visit(NodeID, const IfExpression&) -> void;
    auto visit(NodeID, const IndexExpression&) -> void;
    auto visit(NodeID, const InfiniteLoopExpression&) -> void;
    auto visit(NodeID, const AssignmentExpression&) -> void;
    auto visit(NodeID, const BinaryExpression&) -> void;
    auto visit(NodeID, const DotExpression&) -> void;
    auto visit(NodeID, const RangeExpression&) -> void;
    auto visit(NodeID, const InitializerExpression&) -> void;
    auto visit(NodeID, const LabelExpression&) -> void;
    auto visit(NodeID, const MatchExpression&) -> void;
    auto visit(NodeID, const ReferenceExpression&) -> void;
    auto visit(NodeID, const AddressOfExpression&) -> void;
    auto visit(NodeID, const DereferenceExpression&) -> void;
    auto visit(NodeID, const UnaryExpression&) -> void;
    auto visit(NodeID, const ImplicitAccessExpression&) -> void;
    auto visit(NodeID, const StringExpression&) -> void;
    auto visit(NodeID, const I32Expression&) -> void;
    auto visit(NodeID, const I64Expression&) -> void;
    auto visit(NodeID, const ISizeExpression&) -> void;
    auto visit(NodeID, const U32Expression&) -> void;
    auto visit(NodeID, const U64Expression&) -> void;
    auto visit(NodeID, const USizeExpression&) -> void;
    auto visit(NodeID, const U8Expression&) -> void;
    auto visit(NodeID, const F32Expression&) -> void;
    auto visit(NodeID, const F64Expression&) -> void;
    auto visit(NodeID, const BoolExpression&) -> void;
    auto visit(NodeID, const VoidExpression&) -> void;
    auto visit(NodeID, const UndefinedExpression&) -> void;
    auto visit(NodeID, const ModuleAccessExpression&) -> void;
    auto visit(NodeID, const StructExpression&) -> void;
    auto visit(NodeID, const UnionExpression&) -> void;
    auto visit(NodeID, const WhileLoopExpression&) -> void;
    auto visit(NodeID, const BlockStatement&) -> void;
    auto visit(NodeID, const BreakStatement&) -> void;
    auto visit(NodeID, const ContinueStatement&) -> void;
    auto visit(NodeID, const DeclStatement&) -> void;
    auto visit(NodeID, const DeferStatement&) -> void;
    auto visit(NodeID, const DiscardStatement&) -> void;
    auto visit(NodeID, const ExpressionStatement&) -> void;
    auto visit(NodeID, const ImportStatement&) -> void;
    auto visit(NodeID, const ReturnStatement&) -> void;
    auto visit(NodeID, const TestStatement&) -> void;
    auto visit(NodeID, const UsingStatement&) -> void;
    auto visit(NodeID, stdx::monostate) -> void { fmt::println(out_, "<discarded>"); }

    auto visit(ExplicitTypeID, const IdentifierExpression&) -> void;
    auto visit(ExplicitTypeID, const ModuleAccessExpression&) -> void;
    auto visit(ExplicitTypeID, const DotExpression&) -> void;
    auto visit(ExplicitTypeID, const CallExpression&) -> void;
    auto visit(ExplicitTypeID, const ExplicitFunctionType&) -> void;
    auto visit(ExplicitTypeID, const ExplicitTypeID&) -> void;
    auto visit(ExplicitTypeID, const StructExpression&) -> void;
    auto visit(ExplicitTypeID, const EnumExpression&) -> void;
    auto visit(ExplicitTypeID, const UnionExpression&) -> void;
    auto visit(ExplicitTypeID, const ExplicitArrayType&) -> void;

    template <typename T, typename Func> void dump_container(const T& container, Func&& func) {
        for (auto it{container.begin()}; it != container.end(); ++it) {
            Indent::Guard g{indent_, std::next(it) == container.end()};
            std::forward<Func>(func)(*it);
        }
    }

    template <typename T> void dump_node_list(const T& list) {
        dump_container(list, [this](const auto& node_handle) -> void {
            fmt::print(out_, "{}", indent_.current_branch());
            dump(*node_handle);
        });
    }

    template <> void dump_node_list<Members>(const Members& list) {
        dump_container(list, [this](const MemberHandle& member_handle) -> void {
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
