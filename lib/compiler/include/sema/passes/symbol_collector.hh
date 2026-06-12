#pragma once

#include <string_view>
#include <utility>

#include <gsl/pointers>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/type.hh"
#include "module/error.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include <counter.hh>
#include <option.hh>
#include <result.hh>
#include <types.hh>

namespace ghoti::sema {

// An AST walker that performs 0 type checking
class SymbolCollector {
  public:
    static auto collect_symbols(mod::Module& module, Context& ctx) -> mod::ModuleState;

    template <traits::IndexableID ID> auto collect(ID id) -> void {
        collecting_.ast[id].visit([&](const auto& data) -> void { visit(id, data); });
    }

  private:
    using Scope = SymbolTableStack::Scope;

  private:
    auto visit(ast::NodeID, const ast::ArrayExpression&) -> void;
    auto visit(ast::NodeID, const ast::CallExpression&) -> void;
    auto visit(ast::NodeID, const ast::DoWhileLoopExpression&) -> void;
    template <traits::IndexableID ID> auto visit(ID, const ast::EnumExpression&) -> void;
    auto visit(ast::NodeID, const ast::ForLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::FunctionExpression&) -> void;
    auto visit(ast::NodeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::NodeID, const ast::IfExpression&) -> void;
    auto visit(ast::NodeID, const ast::IndexExpression&) -> void;
    auto visit(ast::NodeID, const ast::InfiniteLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::AssignmentExpression&) -> void;
    auto visit(ast::NodeID, const ast::BinaryExpression&) -> void;
    auto visit(ast::NodeID, const ast::DotExpression&) -> void;
    auto visit(ast::NodeID, const ast::RangeExpression&) -> void;
    auto visit(ast::NodeID, const ast::InitializerExpression&) -> void;
    auto visit(ast::NodeID, const ast::LabelExpression&) -> void;
    auto visit(ast::NodeID, const ast::MatchExpression&) -> void;
    auto visit(ast::NodeID, const ast::ReferenceExpression&) -> void;
    auto visit(ast::NodeID, const ast::AddressOfExpression&) -> void;
    auto visit(ast::NodeID, const ast::DereferenceExpression&) -> void;
    auto visit(ast::NodeID, const ast::UnaryExpression&) -> void;
    auto visit(ast::NodeID, const ast::ImplicitAccessExpression&) -> void;
    auto visit(ast::NodeID, const ast::StringExpression&) -> void;
    auto visit(ast::NodeID, const ast::I32Expression&) -> void;
    auto visit(ast::NodeID, const ast::I64Expression&) -> void;
    auto visit(ast::NodeID, const ast::ISizeExpression&) -> void;
    auto visit(ast::NodeID, const ast::U32Expression&) -> void;
    auto visit(ast::NodeID, const ast::U64Expression&) -> void;
    auto visit(ast::NodeID, const ast::USizeExpression&) -> void;
    auto visit(ast::NodeID, const ast::U8Expression&) -> void;
    auto visit(ast::NodeID, const ast::F32Expression&) -> void;
    auto visit(ast::NodeID, const ast::F64Expression&) -> void;
    auto visit(ast::NodeID, const ast::BoolExpression&) -> void;
    auto visit(ast::NodeID, const ast::VoidExpression&) -> void;
    auto visit(ast::NodeID, const ast::UndefinedExpression&) -> void;
    auto visit(ast::NodeID, const ast::ModuleAccessExpression&) -> void;
    template <traits::IndexableID ID> auto visit(ID, const ast::StructExpression&) -> void;
    template <traits::IndexableID ID> auto visit(ID, const ast::UnionExpression&) -> void;
    auto visit(ast::NodeID, const ast::WhileLoopExpression&) -> void;
    auto visit(ast::NodeID, ast::Discarded) noexcept -> void {}

    auto visit(ast::NodeID, const ast::BlockStatement&) -> void;
    auto visit(ast::NodeID, const ast::BreakStatement&) -> void;
    auto visit(ast::NodeID, const ast::ContinueStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeclStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeferStatement&) -> void;
    auto visit(ast::NodeID, const ast::DiscardStatement&) -> void;
    auto visit(ast::NodeID, const ast::ExpressionStatement&) -> void;

    [[nodiscard]] auto collect_import_payload(const ast::ImportStatement& import_stmt)
        -> std::pair<std::string_view, Result<gsl::not_null<mod::Module*>, mod::Diagnostic>>;

    auto visit(ast::NodeID, const ast::ImportStatement&) -> void;
    auto visit(ast::NodeID, const ast::ReturnStatement&) -> void;
    auto visit(ast::NodeID, const ast::TestStatement&) -> void;
    auto visit(ast::NodeID, const ast::UsingStatement&) -> void;

    auto visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ModuleAccessExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::DotExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void;
    auto visit(ast::ExplicitTypeID, ast::ExplicitTypeID id) -> void { collect(id); }
    auto visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void;

    template <typename... IterPairs>
    [[nodiscard]] auto visit_scopes(TypeKind kind, IterPairs&&... pairs) -> usize {
        const auto  new_idx{ctx_.registry.create()};
        const Scope s{table_stack_, new_idx, table_idx_};
        (..., [&pairs] -> void {
            for (const auto& item : pairs.iterable) { pairs.visitor(item); }
        }());
        last_type_.emplace(ctx_.pool[{kind, types::mut::CONSTANT, new_idx}]);
        return new_idx;
    }

    template <typename SymbolicVariant, typename... Args>
    auto try_declare(std::string_view name, Args&&... args) -> bool {
        const SymbolicVariant node{std::forward<Args>(args)...};
        return ctx_.try_result(ctx_.registry.is_shadowing(table_stack_, collecting_, name, node)) &&
               ctx_.try_result(ctx_.registry.insert_into(table_idx_, collecting_, name, node));
    }

    SymbolCollector(mod::Module& collecting, Context& ctx)
        : collecting_{collecting}, table_idx_{*collecting.root_table_idx}, ctx_{ctx} {
        table_stack_.push(table_idx_);
    }

  private:
    mod::Module&       collecting_;
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context&           ctx_;
    opt::Option<Type&> last_type_;

    DefaultCounter in_expr_scope_;
    DefaultCounter in_function_scope_;
    DefaultCounter in_loop_scope_;
    DefaultCounter in_label_scope_;
};

} // namespace ghoti::sema
