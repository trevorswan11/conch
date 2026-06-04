#pragma once

#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <ankerl/unordered_dense.h>
#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/type.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"
#include "utility.hh"
#include "variant.hh"

namespace ghoti::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    template <traits::IndexableID ID> auto resolve(ID id) -> void {
        std::visit([&](const auto& data) { visit(id, data); }, resolving_.ast[id]);
    }

  private:
    using Scope = SymbolTableStack::Scope;
    using NamedTests =
        ankerl::unordered_dense::map<std::string_view, ast::Handle<ast::NodeKind::TEST_STATEMENT>>;

    // A flag for helper functions to indicate if their resolution was poisoned
    enum class ResolveResult : u8 {
        OK,
        POISONED,
    };

    // A guarded stack that manages the current user-types for function self parameters
    class UserTypeStack {
      public:
        class Guard {
          public:
            Guard(UserTypeStack& s, Type& type) noexcept : stack_{s} { stack_.push(type); }
            ~Guard() { stack_.pop(); }

          private:
            UserTypeStack& stack_;
        };

      public:
        UserTypeStack() noexcept = default;
        ~UserTypeStack()         = default;

        MAKE_MOVE_CONSTRUCTABLE_ONLY(UserTypeStack)

        auto push(Type& type) -> void { stack_.push_back(type); }
        auto pop() noexcept -> void {
            if (!stack_.empty()) { stack_.pop_back(); }
        }

        // Only returns none when there are no types in the stack
        [[nodiscard]] auto peek() const noexcept -> opt::Option<Type&> {
            if (stack_.empty()) { return opt::none; }
            return *stack_.back();
        }

      private:
        std::vector<mem::NonNull<Type>> stack_;
    };

    // Resolves the provided type and unresolves upon desctruction if not committed
    template <typename Resolvee> class CommittableResolution {
      public:
        template <typename... Args>
        CommittableResolution(Type& type, Args&&... resolvee) : type_{type} {
            type_.resolve<Resolvee>(std::forward<Args>(resolvee)...);
        }

        ~CommittableResolution() {
            if (!committed_) { type_.unresolve(); }
        }

        // This action cannot be undone, defer until the very end!
        auto commit() noexcept -> void { committed_ = true; }

      private:
        Type& type_;
        bool  committed_{false};
    };

  private:
    auto visit(ast::NodeID, const ast::ArrayExpression&) -> void;

    // This is meant to be called after the arguments have all been resolved
    template <traits::IndexableID ID>
    [[nodiscard]] auto resolve_builtin_call(ID                            id,
                                            const ast::CallExpression&    call,
                                            const types::BuiltinFunction& builtin)
        -> Result<void, Diagnostic>;

    auto resolve_call_args(std::span<const ast::CallExpression::Argument> args) -> ResolveResult;
    [[nodiscard]] auto get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
        -> mem::NonNull<Type>;
    [[nodiscard]] auto get_call_arg_location(const ast::CallExpression::Argument& arg)
        -> SourceLocation;

    template <traits::IndexableID ID> auto resolve_call(ID, const ast::CallExpression&) -> void;
    auto                                   visit(ast::NodeID, const ast::CallExpression&) -> void;
    auto visit(ast::NodeID, const ast::DoWhileLoopExpression&) -> void;

    [[nodiscard]] auto resolve_members(std::span<mem::NonNull<Type>>      buf,
                                       std::span<const ast::MemberHandle> members)
        -> opt::Option<std::span<mem::NonNull<Type>>>;
    template <traits::IndexableID ID> auto visit(ID, const ast::EnumExpression&) -> void;

    auto visit(ast::NodeID, const ast::ForLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::FunctionExpression&) -> void;

    template <traits::IndexableID ID> auto resolve_symbol(ID, Symbol&) -> void;
    template <traits::IndexableID ID>
    auto resolve_ident(ID, const ast::IdentifierExpression&) -> void;

    auto visit(ast::NodeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::NodeID, const ast::IfExpression&) -> void;
    auto visit(ast::NodeID, const ast::IndexExpression&) -> void;
    auto visit(ast::NodeID, const ast::InfiniteLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::AssignmentExpression&) -> void;
    auto visit(ast::NodeID, const ast::BinaryExpression&) -> void;

    // Attempts to access the given member in the provided user type
    [[nodiscard]] auto
    resolve_user_type_access(Type&                         object_type,
                             ast::IdentifierHandle         member,
                             SourceLocation                object_location,
                             opt::Option<std::string_view> object_name = opt::none)
        -> Result<mem::NonNull<Type>, Diagnostic>;

    // Retrieve's the rightmost identifier name from the accessor
    auto get_rightmost_name(ast::OuterAccessHandle handle) noexcept -> std::string_view;
    template <traits::IndexableID ID> auto resolve_dot(ID, const ast::DotExpression&) -> void;

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

    template <traits::IndexableID ID>
    auto resolve_module_access(ID, const ast::ModuleAccessExpression&) -> void;
    auto visit(ast::NodeID, const ast::ModuleAccessExpression&) -> void;

    template <traits::IndexableID ID> auto visit(ID, const ast::StructExpression&) -> void;
    template <traits::IndexableID ID> auto visit(ID, const ast::UnionExpression&) -> void;
    auto visit(ast::NodeID, const ast::WhileLoopExpression&) -> void;

    auto visit(ast::NodeID, const ast::BlockStatement&) -> void;

    // Returns `true` if the resolution was successful
    [[nodiscard]] auto resolve_control_flow_label(opt::Option<ast::IdentifierHandle> label,
                                                  std::string_view                   stmt_name)
        -> Result<opt::Option<Symbol&>, Diagnostic>;

    auto visit(ast::NodeID, const ast::BreakStatement&) -> void;
    auto visit(ast::NodeID, const ast::ContinueStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeclStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeferStatement&) -> void;
    auto visit(ast::NodeID, const ast::DiscardStatement&) -> void;
    auto visit(ast::NodeID, const ast::ExpressionStatement&) -> void;
    auto visit(ast::NodeID, const ast::ImportStatement&) -> void;
    auto visit(ast::NodeID, const ast::ReturnStatement&) -> void;
    auto visit(ast::NodeID, const ast::TestStatement&) -> void;
    auto visit(ast::NodeID, const ast::UsingStatement&) -> void;
    auto visit(ast::NodeID, const Unit&) noexcept -> void {}

    // Creates a potentially new type with the id-stored modifiers
    auto apply_explicit_modifiers(ast::ExplicitTypeID id, Type& inner_type) -> Type&;

    auto visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ModuleAccessExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::DotExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitTypeID) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void;

    // Looks up the symbol by name in the current index ONLY. Changes no state on failure
    auto resolve_symbol_info(ast::IdentifierHandle handle, opt::Option<SymbolKind> kind)
        -> opt::Option<Symbol&>;

    TypeResolver(mod::Module& resolving, Context& ctx)
        : resolving_{resolving}, table_idx_{*resolving.root_table_idx}, ctx_{ctx} {
        ASSERT(ctx.prelude_index, "TypeResolver must be used after prelude-injection");
        table_stack_.push(*ctx_.prelude_index);
        table_stack_.push(table_idx_);
    }

  private:
    mod::Module&     resolving_;
    usize            table_idx_;
    SymbolTableStack table_stack_;
    UserTypeStack    user_type_stack_;
    UserTypeStack    implicit_type_stack_;
    NamedTests       named_tests_; // Named tests of the currently resolving module

    Context&           ctx_;
    opt::Option<Type&> last_type_;
};

} // namespace ghoti::sema
