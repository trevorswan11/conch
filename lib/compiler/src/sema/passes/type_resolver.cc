#include "sema/passes/type_resolver.hh"

#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gsl/pointers>
#include <gsl/span>

#include "ast/expression.hh"
#include "ast/format.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/type.hh"
#include "ast/visitor.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"
#include "syntax/token_type.hh"

#include <assert.hh>
#include <diagnostic.hh>
#include <option.hh>
#include <profiler.hh>
#include <result.hh>
#include <types.hh>
#include <utility.hh>
#include <variant.hh>

namespace ghoti::sema {

auto TypeResolver::resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState {
    PROFILE_FUNCTION();

    // Poisoned collection should flush the diagnostics
    const auto poisoned_collection{module.state == mod::ModuleState::POISONED_SYMBOL_COLLECTION};
    if (poisoned_collection) { module.print_diagnostics(ctx.error_stream); }

    if (module.is_resolvable()) {
        module.state = poisoned_collection ? mod::ModuleState::POISONED_TYPE_RESOLVING
                                           : mod::ModuleState::TYPE_RESOLVING;
        ctx.inject_prelude();

        TypeResolver resolver{module, ctx};
        for (const auto& node : module.ast) { resolver.resolve(node); }

        if (!ctx.diagnostics.empty() || module.is_poisoned()) {
            return module.error_out(std::move(ctx.diagnostics),
                                    mod::ModuleState::POISONED_TYPE_RESOLVED);
        }
        module.state = mod::ModuleState::TYPE_RESOLVED;
    }
    return module.state;
}

// Performs the resolution and poison check & bubble & return operation
#define TRY_RESOLVE(resolvable_expr)                                                       \
    do {                                                                                   \
        resolve(resolvable_expr);                                                          \
        if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); } \
    } while (0)

namespace {

[[nodiscard]] auto incomplete_array_item(const SourceLocation& location) -> Diagnostic {
    return Diagnostic{
        "Array elements cannot have an incomplete type", Error::CYCLIC_DEPENDENCY, location};
}

} // namespace

auto TypeResolver::visit(ast::NodeID id, const ast::ArrayExpression& array) -> void {
    PROFILE_FUNCTION();
    for (const auto& item : array.items) { resolve(item); }
    resolve(array.item_explicit_type);
    auto& item_type{*last_type_.take()};

    if (!item_type.is_resolved()) {
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            incomplete_array_item(resolving_.ast.location_of(array.item_explicit_type))));
    }

    last_type_.emplace(
        ctx_.get_array(types::mut::CONSTANT, array.null_terminated, array.items.size(), item_type));
    resolving_.set_sema_type(id, *last_type_);
}

template <traits::IndexableID ID>
[[nodiscard]] auto TypeResolver::resolve_builtin_call(ID                            id,
                                                      const ast::CallExpression&    call,
                                                      const types::BuiltinFunction& builtin)
    -> Result<void, Diagnostic> {
    ASSERT(call.function.is<ast::IdentifierExpression>(), "Builtin function must be a raw ident");
    const auto& params{builtin.params};
    if (call.arguments.size() != params.size()) {
        return make_sema_err(fmt::format("Builtin expects {} arguments, found {}",
                                         params.size(),
                                         call.arguments.size()),
                             Error::ARITY_MISMATCH,
                             resolving_.ast.location_of(call.function));
    }

    // There must be an actual builtin present with a token id
    const auto builtin_id{call.function->get_token_type()};
    ASSERT(syntax::get_builtin_opt(builtin_id), "Cannot resolve non-builtin function");

    using syntax::TokenType;
    gsl::not_null<Type*> return_type = &ctx_.get_poison();

    // Indexing can be done freely as arity is already validated
    switch (builtin_id) {
    case TokenType::BUILTIN_ALIGN_CAST:
    case TokenType::BUILTIN_PTR_CAST:
    case TokenType::BUILTIN_BIT_CAST:
    case TokenType::BUILTIN_AS:         {
        // These builtins take in a resulting type to cast to
        return_type = get_resolved_call_arg_type(call.arguments[0]);
        break;
    }
    case TokenType::BUILTIN_CONST_CAST: {
        // Pointer/reference is checked since it is an invariant of the cast
        auto& expr_type{*get_resolved_call_arg_type(call.arguments[0])};
        if (expr_type.get_kind() == TypeKind::POINTER ||
            expr_type.get_kind() == TypeKind::REFERENCE) {
            return_type = ctx_.pool.strip_const(expr_type);
        } else {
            return make_sema_err(fmt::format("Expected pointer or reference type; found '{}'",
                                             type_kind_display_name(expr_type.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }
        break;
    }
    case TokenType::BUILTIN_VOLATILE_CAST: {
        auto& expr_type{*get_resolved_call_arg_type(call.arguments[0])};
        return_type = ctx_.pool.strip_volatile(expr_type);
        break;
    }
    case TokenType::BUILTIN_INT_FROM_PTR:
    case TokenType::BUILTIN_ALIGN_OF:
    case TokenType::BUILTIN_SIZE_OF:
    case TokenType::BUILTIN_CLZ:
    case TokenType::BUILTIN_CTZ:
    case TokenType::BUILTIN_POP_COUNT:    {
        ASSERT(builtin.return_type.get_kind() == TypeKind::USIZE);
        return_type = &builtin.return_type;
        break;
    }
    // @typeOf returns a type as per documentation, but it's not the literal `type` type
    case TokenType::BUILTIN_TYPE_OF: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::TYPE);
        auto&       instance_type{*get_resolved_call_arg_type(call.arguments[0])};
        const auto& instance_data{instance_type.get_data()};
        if (instance_data.is<types::DeferredCall>() || instance_data.is<types::DeferredArray>()) {
            return_type = ctx_.pool[{TypeKind::TYPE, types::mut::CONSTANT, &call}];
            return_type->resolve_if<types::DeferredCall>(call);
        } else {
            return_type = ctx_.pool[{TypeKind::TYPE, types::mut::CONSTANT, instance_type}];
            return_type->resolve_if<types::MetaType>(instance_type);
        }
        break;
    }
    // @this returns a type as per docs, but it's really a structural type with full determinism
    case TokenType::BUILTIN_THIS:
        if (const auto user_type{user_type_stack_.peek()}) {
            return_type = user_type.get();
            break;
        }

        return make_sema_err("@this() may only be used inside of structs, unions, and enums",
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id));
    case TokenType::BUILTIN_PTR_FROM_ARRAY: {
        auto& array_type{*get_resolved_call_arg_type(call.arguments[0])};
        auto& type_data{array_type.get_data()};
        // The new type uses a new key to align with normal pointer creation
        if (const auto array_data{type_data.as_opt<types::Array>()}) {
            return_type = &ctx_.get_pointer(types::mut::CONSTANT, array_data->underlying);
        } else if (const auto deferred_data{type_data.as_opt<types::DeferredArray>()}) {
            return_type = &ctx_.get_pointer(types::mut::CONSTANT, deferred_data->underlying);
        } else {
            return make_sema_err(fmt::format("Expected an array-yielding expression; found '{}'",
                                             type_kind_display_name(array_type.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }
        break;
    }
    case TokenType::BUILTIN_PTR_FROM_INT: {
        auto& requested_output{*get_resolved_call_arg_type(call.arguments[0])};
        if (requested_output.get_kind() == TypeKind::POINTER) {
            return_type = &requested_output;
            break;
        }
        return make_sema_err(fmt::format("Expected a pointer type; found '{}'",
                                         type_kind_display_name(requested_output.get_kind())),
                             Error::TYPE_MISMATCH,
                             get_call_arg_location(call.arguments[0]));
    }
    case TokenType::BUILTIN_SLICE_FROM_PTR: {
        auto& ptr_type{*get_resolved_call_arg_type(call.arguments[0])};
        if (const auto ptr_data{ptr_type.get_data().as_opt<types::Pointer>()}) {
            // The resulting slice isn't null terminated since the pointer gives no guarantee
            return_type = &ctx_.get_slice(types::mut::CONSTANT, false, ptr_data->underlying);
            break;
        }

        return make_sema_err(fmt::format("Expected a pointer-yielding expression; found '{}'",
                                         type_kind_display_name(ptr_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             get_call_arg_location(call.arguments[0]));
    }
    case TokenType::BUILTIN_TAG_NAME: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::SLICE);
        return_type = &builtin.return_type;
        break;
    }
    case TokenType::BUILTIN_MEMCPY:
    case TokenType::BUILTIN_MEMSET:
    case TokenType::BUILTIN_MEMMOVE: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::VOID);
        return_type = &builtin.return_type;
        break;
    }
    // Many of these return @typeOf(expression) which is trivial
    case TokenType::BUILTIN_MUL_ADD:
    case TokenType::BUILTIN_SQRT:
    case TokenType::BUILTIN_SIN:
    case TokenType::BUILTIN_COS:
    case TokenType::BUILTIN_TAN:
    case TokenType::BUILTIN_EXP:
    case TokenType::BUILTIN_EXP2:
    case TokenType::BUILTIN_LOG:
    case TokenType::BUILTIN_LOG2:
    case TokenType::BUILTIN_LOG10:
    case TokenType::BUILTIN_ABS:
    case TokenType::BUILTIN_FLOOR:
    case TokenType::BUILTIN_CEIL:    {
        return_type = get_resolved_call_arg_type(call.arguments[0]);
        break;
    }
    case TokenType::BUILTIN_PANIC: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::NORETURN);
        return_type = &builtin.return_type;
        break;
    }
    default: UNREACHABLE("Unimplemented builtin type resolution");
    }

    resolving_.set_sema_type(id, *return_type);
    last_type_.emplace(*return_type);
    return {};
}

template auto TypeResolver::resolve_builtin_call<ast::NodeID>(ast::NodeID,
                                                              const ast::CallExpression&,
                                                              const types::BuiltinFunction&)
    -> Result<void, Diagnostic>;
template auto TypeResolver::resolve_builtin_call<ast::ExplicitTypeID>(ast::ExplicitTypeID,
                                                                      const ast::CallExpression&,
                                                                      const types::BuiltinFunction&)
    -> Result<void, Diagnostic>;

auto TypeResolver::resolve_call_args(gsl::span<const ast::CallExpression::Argument> args)
    -> ResolveResult {
    bool any_poison{false};
    for (const auto& arg : args) {
        any_poison |= arg.visit([this](auto id) -> bool {
            resolve(id);
            return last_type_.take()->is_poison();
        });
    }
    return any_poison ? ResolveResult::POISONED : ResolveResult::OK;
}

auto TypeResolver::get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
    -> gsl::not_null<Type*> {
    return &arg.visit(
        [this](ast::ExpressionHandle id) -> auto& {
            // Labels store their actual type in nested node data
            if (const auto label{resolving_.ast.get_as_opt<ast::LabelExpression>(id)}) {
                auto type{resolving_.get_sema_type_opt(label->name)};
                ASSERT(type, "Labeled call arg was not typed");
                return *type;
            }

            auto type{resolving_.get_sema_type_opt(id)};
            ASSERT(type, "Call argument was not typed");
            return *type;
        },
        [this](ast::ExplicitTypeID id) -> auto& {
            auto type{resolving_.get_sema_type_opt(id)};
            ASSERT(type, "Call argument was not typed");
            return *type;
        });
}

auto TypeResolver::get_call_arg_location(const ast::CallExpression::Argument& arg)
    -> SourceLocation {
    return arg.visit([this](auto id) -> SourceLocation { return resolving_.ast.location_of(id); });
}

template <traits::IndexableID ID>
auto TypeResolver::resolve_call(ID id, const ast::CallExpression& call) -> void {
    // The call can only yield a non-poison type if the function is valid
    resolve(call.function);
    auto& callee_type{*last_type_.take()};
    if (callee_type.is_poison()) {
        resolve_call_args(call.arguments);
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
    resolving_.set_sema_type(call.function, callee_type);

    // Verify that the type in the function is callable and store the return type
    auto& callee_data{callee_type.get_data()};
    if (const auto function_type{callee_data.as_opt<types::Function>()}) {
        const auto has_implicit_self{function_type->has_self &&
                                     resolving_.ast.get_as_opt<ast::DotExpression>(call.function)};

        // Check the arity of the function against params before resetting last type
        const auto& params{function_type->params};
        const usize param_offset{has_implicit_self ? 1UZ : 0UZ};
        const auto  expected_arity{params.size() - param_offset};
        if (call.arguments.size() != expected_arity) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format(
                    "Expected {} arguments, found {}", expected_arity, call.arguments.size()),
                Error::ARITY_MISMATCH,
                resolving_.ast.location_of(call.function)));
        }

        bool any_arg_poison{false};
        for (auto [param_type, arg] :
             std::views::zip(function_type->params.subspan(param_offset), call.arguments)) {
            const StructuralGuard g{implicit_type_stack_, *param_type};
            any_arg_poison |= arg.visit([this](auto id) -> bool {
                resolve(id);
                return last_type_.take()->is_poison();
            });
        }
        if (any_arg_poison) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        // Functions that return a type cannot be resolved until the constant evaluator
        if (function_type->return_type.get_kind() == TypeKind::TYPE) {
            auto& deferred_type{*ctx_.pool[{TypeKind::TYPE, types::mut::CONSTANT, &call}]};
            deferred_type.resolve_if<types::DeferredCall>(call);

            resolving_.set_sema_type(id, deferred_type);
            return last_type_.emplace(deferred_type);
        }

        // Only arity is checked since the type checker will handle the rest
        resolving_.set_sema_type(id, function_type->return_type);
        last_type_.emplace(function_type->return_type);
    } else if (const auto builtin_type{callee_data.as_opt<types::BuiltinFunction>()}) {
        // There's no need to check any further if the arguments are poisoned
        if (resolve_call_args(call.arguments) == ResolveResult::POISONED) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id));
        }

        // Poison the call if there's an error early
        auto result{resolve_builtin_call(id, call, *builtin_type)};
        if (!result) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result.error())));
        }
    } else {
        return last_type_.emplace(ctx_.poison_node(resolving_,
                                                   id,
                                                   "Expression is not callable",
                                                   Error::NON_CALLABLE_EXPRESSION,
                                                   resolving_.ast.location_of(call.function)));
    }
}

VISITOR_TEMPLATE_INIT(TypeResolver, resolve_call, const ast::CallExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::CallExpression& call) -> void {
    PROFILE_FUNCTION();
    resolve_call(id, call);
}

auto TypeResolver::visit(ast::NodeID id, const ast::DoWhileLoopExpression& do_while) -> void {
    PROFILE_FUNCTION();

    // The loop itself holds the block index, not the block
    auto& loop_type{resolving_.get_sema_type(id)};
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};
        const auto& block{resolving_.ast.get_as<ast::BlockStatement>(do_while.block)};
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    TRY_RESOLVE(do_while.condition);
    last_type_.emplace(loop_type);
}

auto TypeResolver::resolve_members(gsl::span<Type*> buf, gsl::span<const ast::MemberHandle> members)
    -> opt::Option<gsl::span<Type*>> {
    // Only poison the enum once all members are collected
    for (usize i{0}; const auto& member : members) {
        // The resolved statement type is in last_type_ unlike in the symbol collector
        resolve(*member);
        auto& member_type{resolving_.get_sema_type(member)};
        if (member_type.is_poison()) { return opt::none; }
        buf[i++] = &member_type;
    }
    return buf;
}

template <traits::IndexableID ID>
auto TypeResolver::visit(ID id, const ast::EnumExpression& enum_expr) -> void {
    PROFILE_FUNCTION();
    if (enum_expr.underlying) { resolve(*enum_expr.underlying); }

    auto&                 enum_type{resolving_.get_sema_type(id)};
    const Scope           s{table_stack_, enum_type.get_symbol_table_idx(), table_idx_};
    const StructuralGuard g{user_type_stack_, enum_type};

    // The underlying type defaults to an i32 as it would in C or C++
    auto& underlying_type{enum_expr.underlying ? resolving_.get_sema_type(*enum_expr.underlying)
                                               : ctx_.get_builtin_resolved_type(TypeKind::I32)};

    for (const auto& [name, value] : enum_expr.enumerations) {
        if (value) { TRY_RESOLVE(*value); }

        // The value's type doesn't contribute to the actual enumeration type
        const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(name)};
        auto        symbol{ctx_.registry.get_from_opt(table_idx_, ident.name)};
        if (!symbol) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }
        resolving_.set_sema_type(name, underlying_type);
        symbol->set_kind(SymbolKind::VALUE);
        symbol->set_status(SymbolStatus::RESOLVED);
    }

    auto member_types{ctx_.pool.get_many_unsafe(enum_expr.members.size())};
    CommittableResolution<types::Enum> resolution{enum_type,
                                                  enum_expr.enumerations,
                                                  enum_expr.non_exhaustive,
                                                  underlying_type,
                                                  member_types,
                                                  resolving_};
    if (!resolve_members(member_types, enum_expr.members)) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }

    resolution.commit();
    last_type_.emplace(enum_type);
}

VISITOR_TEMPLATE_INIT(TypeResolver, visit, const ast::EnumExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::ForLoopExpression& for_expr) -> void {
    PROFILE_FUNCTION();
    ASSERT(for_expr.iterables.size() == for_expr.captures.size());

    // The loop itself holds the block index which houses captures, not the block
    auto& loop_type{resolving_.get_sema_type(id)};
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};

        // The captures must be paired with the iterables inner types (shallow type check)
        for (const auto& [capture, iterable] :
             std::views::zip(for_expr.captures, for_expr.iterables)) {
            TRY_RESOLVE(iterable);
            auto& iterable_type{*last_type_.take()};
            resolving_.set_sema_type(iterable, iterable_type);

            // Assign types unconditionally since ignoring discards saves no space
            auto& iterable_data{iterable_type.get_data()};
            if (const auto array{iterable_data.as_opt<types::Array>()}) {
                resolving_.set_sema_type(capture.payload, array->underlying);
            } else if (const auto deferred{iterable_data.as_opt<types::DeferredArray>()}) {
                resolving_.set_sema_type(capture.payload, deferred->underlying);
            } else if (const auto slice{iterable_data.as_opt<types::Slice>()}) {
                resolving_.set_sema_type(capture.payload, slice->underlying);
            } else {
                return last_type_.emplace(ctx_.poison_node(
                    resolving_,
                    id,
                    fmt::format("Iterables may only be arrays or slices; found '{}'",
                                type_kind_display_name(iterable_type.get_kind())),
                    Error::TYPE_MISMATCH,
                    resolving_.ast.location_of(iterable)));
            }

            if (capture.payload.is<ast::IdentifierExpression>()) {
                resolve_symbol_info(capture.payload, SymbolKind::VALUE);
            }
        }
        const auto& block{resolving_.ast.get_as<ast::BlockStatement>(for_expr.block)};
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    if (for_expr.non_break) { TRY_RESOLVE(*for_expr.non_break); }
    last_type_.emplace(loop_type);
}

namespace {

[[nodiscard]] auto mutability_from_type_modifier(ast::TypeModifier modifier) noexcept
    -> opt::Option<types::MutabilityModifiers> {
    using Modifier = ast::TypeModifier::Modifier;
    switch (modifier.get_raw()) {
    case Modifier::VALUE:        return opt::none;
    case Modifier::REF:          return types::mut::CONSTANT;
    case Modifier::MUT_REF:      return types::mut::MUTABLE;
    case Modifier::PTR:          return types::mut::CONSTANT;
    case Modifier::MUT_PTR:      return types::mut::MUTABLE;
    case Modifier::VOLATILE:     return types::mut::CONSTANT_VOLATILE;
    case Modifier::MUT_VOLATILE: return types::mut::VOLATILE;
    }
}

} // namespace

auto TypeResolver::visit(ast::NodeID id, const ast::FunctionExpression& fn) -> void {
    PROFILE_FUNCTION();

    // The entire function lives inside of its preallocated scope
    auto&       fn_type{resolving_.get_sema_type(id)};
    const Scope s{table_stack_, fn_type.get_symbol_table_idx(), table_idx_};

    const auto true_param_size{fn.parameters.size() + (fn.self ? 1 : 0)};
    auto       param_types{ctx_.pool.get_many_unsafe(true_param_size)};
    usize      param_idx{0};

    if (fn.self) {
        // If self is valid here, then follow a similar tune to the type resolvers
        if (const auto user_type{user_type_stack_.peek()}) {
            const auto modifier{fn.self->modifier};
            const auto mutability{mutability_from_type_modifier(modifier)};

            if (user_type->is_poison()) {
                return last_type_.emplace(ctx_.poison_node(resolving_, id));
            }

            if (!mutability) {
                last_type_.emplace(*user_type);
                resolving_.set_sema_type(fn.self->name, *last_type_);
            } else {
                // Imprint generally here since a new type is always created (else unreachable)
                auto new_key{user_type->get_key()};
                new_key.set_mut(*mutability);
                new_key.imprint(*user_type);

                if (modifier.is_ptr()) {
                    new_key.set_kind(TypeKind::POINTER);
                    last_type_.emplace(ctx_.pool[new_key]);
                    last_type_->resolve_if<types::Pointer>(*user_type);
                    resolving_.set_sema_type(fn.self->name, *last_type_);
                } else if (modifier.is_ref()) {
                    new_key.set_kind(TypeKind::REFERENCE);
                    last_type_.emplace(ctx_.pool[new_key]);
                    last_type_->resolve_if<types::Reference>(*user_type);
                    resolving_.set_sema_type(fn.self->name, *last_type_);
                } else {
                    UNREACHABLE("Self parameter has a modifier that slipped through the parser");
                }
            }
        } else {
            resolve_symbol_info(fn.self->name, SymbolKind::POISONED);
            return last_type_.emplace(
                ctx_.poison_node(resolving_,
                                 id,
                                 "Self parameters may only be used inside member functions",
                                 Error::ILLEGAL_SELF_PARAMETER,
                                 resolving_.ast.location_of(fn.self->name)));
        }

        param_types[param_idx++] = last_type_.take();
        resolve_symbol_info(fn.self->name, SymbolKind::VALUE);
    }

    // Every parameter contributes to the resolution but not the type key due to unique idx
    for (const auto& param : fn.parameters) {
        TRY_RESOLVE(param.explicit_type);

        auto& param_type{*last_type_.take()};
        param_types[param_idx++] = &param_type;
        resolving_.set_sema_type(param.name, param_type);
        resolve_symbol_info(param.name, SymbolKind::VALUE);
    }

    TRY_RESOLVE(fn.explicit_return_type);
    auto& return_type{*last_type_.take()};
    ASSERT(!fn_type.is_resolved(), "Valued function must not be resolved");

    // This function type MUST be unique so the self-ness is guaranteed to be accurate
    fn_type.resolve<types::Function>(fn.self.has_value(), param_types, return_type);
    const auto& block{resolving_.ast.get_as<ast::BlockStatement>(fn.body)};
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(fn_type);
}

auto TypeResolver::resolve_symbol_info(ast::IdentifierHandle handle, opt::Option<SymbolKind> kind)
    -> opt::Option<Symbol&> {
    const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(handle)};
    return ctx_.registry.get_from_opt(table_idx_, ident.name)
        .transform([&](Symbol& symbol) -> auto& {
            if (kind) { symbol.set_kind(*kind); }
            symbol.set_status(SymbolStatus::RESOLVED);
            return symbol;
        });
}

namespace {

template <typename T>
concept HasName = requires(T t) { t.name; };

template <typename T>
concept HasType = requires(T t) { t.explicit_type; };

template <typename T>
concept HasNameOnly = HasName<T> && !HasType<T>;

template <typename T>
concept HasBothNameAndType = HasName<T> && HasType<T>;

} // namespace

namespace {

// Forwards an incomplete aggregate type to safely break cycles if possible
[[nodiscard]] auto forward_type(const mod::Module&             target_mod,
                                opt::Option<ast::TypeModifier> mod,
                                Symbol& symbol) noexcept -> opt::Option<Type&> {
    const auto node{symbol.get_data().as_opt<symbols::Node>()};
    if (!node) { return opt::none; }

    if (const auto decl{target_mod.ast.get_as_opt<ast::DeclStatement>(*node)}) {
        if (!decl->value) { return opt::none; }

        const auto is_aggregate{decl->value->is<ast::StructExpression>() ||
                                decl->value->is<ast::UnionExpression>() ||
                                decl->value->is<ast::EnumExpression>()};
        if ((!mod || (!mod->is_ptr() && !mod->is_ref())) && !is_aggregate) { return opt::none; }
        return target_mod.get_sema_type_opt(*decl->value);
    }

    if (const auto alias{target_mod.ast.get_as_opt<ast::UsingStatement>(*node)}) {
        return target_mod.get_sema_type_opt(alias->explicit_type);
    }
    return opt::none;
}

} // namespace

template <traits::IndexableID ID> auto TypeResolver::resolve_symbol(ID id, Symbol& symbol) -> void {
    auto& symbol_data{symbol.get_data()};
    switch (symbol.get_status()) {
    case SymbolStatus::RESOLVED:
        // Identifier handles are not unique in the tree, but their symbol can be used to find root
        resolving_.set_sema_type_if(
            id,
            symbol_data.visit(
                [](symbols::Builtin& builtin) -> Type& { return builtin.get_type(); },
                [this](symbols::Label& label) -> Type& {
                    const auto defn{label.get_definition()};
                    ASSERT(resolving_.has_sema_type(defn), "Resolved node has no type");
                    return resolving_.get_sema_type(defn);
                },
                [this](auto& sym) -> Type& {
                    ASSERT(resolving_.has_sema_type(sym),
                           "Directly indexable symbol was never typed");
                    return resolving_.get_sema_type(sym);
                },
                [this](HasBothNameAndType auto& sym) -> Type& {
                    ASSERT(resolving_.has_sema_type(sym.name), "Symbol was never typed");
                    auto& type{resolving_.get_sema_type(sym.name)};
                    ASSERT(type == resolving_.get_sema_type_opt(sym.explicit_type),
                           "Symbol was resolved with mismatched type");
                    return type;
                },
                [this](HasNameOnly auto& sym) -> Type& {
                    ASSERT(resolving_.has_sema_type(sym.name), "Name-only sym was never typed");
                    return resolving_.get_sema_type(sym.name);
                },
                [this](symbols::ForLoopCapture& capture) -> Type& {
                    ASSERT(capture.payload.is<ast::IdentifierExpression>(),
                           "Capture payload must be an ident");
                    ASSERT(resolving_.has_sema_type(capture.payload),
                           "For loop capture was never typed");
                    return resolving_.get_sema_type(capture.payload);
                }));
        break;
    case SymbolStatus::RESOLVING:
        if (const auto forwarded_type{forward_type(resolving_, opt::none, symbol)}) {
            resolving_.set_sema_type_if(id, *forwarded_type);
            return last_type_.emplace(*forwarded_type);
        }

        ctx_.poison_symbol(symbol);
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            fmt::format("'{}' is used during its own resolution", symbol.get_name()),
            Error::CYCLIC_DEPENDENCY,
            resolving_.ast.location_of(id)));
    case SymbolStatus::UNRESOLVED: {
        symbol.set_status(SymbolStatus::RESOLVING);

        // All other symbol data kinds are independently resolved
        const auto node{symbol_data.as_opt<symbols::Node>()};
        ASSERT(node, "Unresolved symbol is not AST-associated");
        resolve(*node);
        resolving_.set_sema_type(id, *last_type_.take());
        break;
    }
    default: UNREACHABLE("Symbol status should only be 1 of 3 states");
    }

    if (symbol.get_kind() == SymbolKind::POISONED) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
    last_type_.emplace(resolving_.get_sema_type(id));
}

VISITOR_TEMPLATE_INIT(TypeResolver, resolve_symbol, Symbol&)

template <traits::IndexableID ID>
auto TypeResolver::resolve_ident(ID id, const ast::IdentifierExpression& ident) -> void {
    const auto name{ident.name};
    auto       symbol_opt{ctx_.registry.lookup(table_stack_, name)};

    // Check for an undeclared identifier and poison the ident
    if (!symbol_opt) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Use of undeclared identifier '{}'", name),
                             Error::UNDECLARED_IDENTIFIER,
                             resolving_.ast.location_of(id)));
    }
    resolve_symbol(id, *symbol_opt);
}

VISITOR_TEMPLATE_INIT(TypeResolver, resolve_ident, const ast::IdentifierExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::IdentifierExpression& ident) -> void {
    PROFILE_FUNCTION();
    resolve_ident(id, ident);
}

auto TypeResolver::visit(ast::NodeID id, const ast::IfExpression& if_expr) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(if_expr.condition);
    TRY_RESOLVE(if_expr.consequence);

    // There can only be a non-void type with an alternate but this is for pass 3
    auto& branch_type{*last_type_.take()};
    if (if_expr.alternate) { TRY_RESOLVE(*if_expr.alternate); }
    resolving_.set_sema_type(id, branch_type);
    last_type_.emplace(branch_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::IndexExpression& index) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(index.array);
    auto& array_type{*last_type_.take()};
    auto& array_data{array_type.get_data()};

    if (const auto slice{array_data.as_opt<types::Slice>()}) {
        last_type_.emplace(slice->underlying);
    } else if (const auto array{array_data.as_opt<types::Array>()}) {
        last_type_.emplace(array->underlying);
    } else if (const auto deferred{array_data.as_opt<types::DeferredArray>()}) {
        last_type_.emplace(deferred->underlying);
    } else if (const auto pointer{array_data.as_opt<types::Pointer>()}) {
        last_type_.emplace(pointer->underlying);
    } else {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Can only index slices, arrays, and pointers; found '{}'",
                                         type_kind_display_name(array_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(index.array)));
    }

    auto& single_item_type{*last_type_.take()};
    TRY_RESOLVE(index.index);
    auto& access_type{*last_type_.take()};

    // There may be a slice accessor which results in a slice type
    if (access_type.get_data().is<types::Slice>()) {
        last_type_.emplace(ctx_.get_slice(types::mut::CONSTANT, false, single_item_type));
    } else {
        last_type_.emplace(single_item_type);
    }

    auto& result_type{*last_type_.take()};
    resolving_.set_sema_type(id, result_type);
    last_type_.emplace(result_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::InfiniteLoopExpression& loop) -> void {
    PROFILE_FUNCTION();
    auto&       loop_type{resolving_.get_sema_type(id)};
    const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};

    // Just an abridged normal loop handler
    const auto& block{resolving_.ast.get_as<ast::BlockStatement>(loop.block)};
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(loop_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::AssignmentExpression& assign) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(assign.lhs);
    auto& lhs_type{*last_type_.take()};
    {
        const StructuralGuard g{implicit_type_stack_, lhs_type};
        TRY_RESOLVE(assign.rhs);
    }

    // Only pass 3 can verify assignment allowance due to mutability semantics
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::BinaryExpression& binary) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(binary.lhs);
    TRY_RESOLVE(binary.rhs);
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::resolve_structural_access(Type&                         object_type,
                                             ast::IdentifierHandle         member,
                                             SourceLocation                object_location,
                                             opt::Option<std::string_view> object_name)
    -> Result<gsl::not_null<Type*>, Diagnostic> {
    // Early validation to simplify error handling
    auto&      object_data{object_type.get_data()};
    const auto enum_type{object_data.as_opt<types::Enum>()};
    const auto struct_type{object_data.as_opt<types::Struct>()};
    const auto union_type{object_data.as_opt<types::Union>()};
    if (!enum_type && !struct_type && !union_type) {
        return make_sema_err(
            fmt::format(
                "Can only access inner objects inside of structs, unions, and enums; found '{}'",
                type_kind_display_name(object_type.get_kind())),
            Error::TYPE_MISMATCH,
            object_location);
    }

    ASSERT(object_type.has_symbol_table_idx(), "Structural should have a resolved table index");
    const auto table_idx{object_type.get_symbol_table_idx()};
    auto&      table{ctx_.registry.get(table_idx)};

    const auto& member_ident{resolving_.ast.get_as<ast::IdentifierExpression>(member)};
    const Scope s{table_stack_, table_idx, table_idx_};
    auto        symbol_proxy{table.get_proxy_opt(member_ident.name)};

    if (!symbol_proxy) {
        return make_sema_err(
            object_name
                .transform([&](std::string_view name) -> std::string {
                    return fmt::format(
                        "Type '{}' has no field named '{}'", name, member_ident.name);
                })
                .value_or(fmt::format("Type has no field named '{}'", member_ident.name)),
            Error::UNDECLARED_IDENTIFIER,
            resolving_.ast.location_of(member));
    }

    auto& [member_symbol, member_idx] = *symbol_proxy;
    gsl::not_null<Type*> result_type  = &ctx_.get_poison();
    if (member_symbol.get_kind() == SymbolKind::POISONED) { return result_type; }

    if (enum_type) { return &enum_type->type_at(member_idx, object_type); }
    if (struct_type) { return &struct_type->type_at(member_idx); }
    if (union_type) { return &union_type->type_at(member_idx); }
    UNREACHABLE("Error handling failed to catch invalid type");
}

auto TypeResolver::get_rightmost_name(ast::OuterAccessHandle handle) const noexcept
    -> std::string_view {
    auto current{handle};
    while (true) {
        if (const auto ident{resolving_.ast.get_as_opt<ast::IdentifierExpression>(current)}) {
            return ident->name;
        }

        if (const auto scope{resolving_.ast.get_as_opt<ast::ModuleAccessExpression>(current)}) {
            current = scope->inner;
            continue;
        }

        if (const auto dot{resolving_.ast.get_as_opt<ast::DotExpression>(current)}) {
            current = dot->member;
            continue;
        }
        UNREACHABLE("OuterAccessHandle has violated an invariant of the Handle class");
    }
}

template <traits::IndexableID ID>
auto TypeResolver::resolve_dot(ID id, const ast::DotExpression& dot) -> void {
    resolve(dot.object);
    if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); }
    auto& object_type{*last_type_.take()};

    auto result{resolve_structural_access(object_type,
                                          dot.member,
                                          resolving_.ast.location_of(dot.object),
                                          get_rightmost_name(dot.object))};
    if (!result) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result).error()));
    }

    // The structural resolver returns poisoned types in error conditions which can be bubbled here
    auto& member_type{*result.value()};
    resolving_.set_sema_type(dot.member, member_type);
    resolving_.set_sema_type(id, member_type);
    last_type_.emplace(member_type);
}

VISITOR_TEMPLATE_INIT(TypeResolver, resolve_dot, const ast::DotExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::DotExpression& dot) -> void {
    PROFILE_FUNCTION();
    resolve_dot(id, dot);
}

auto TypeResolver::visit(ast::NodeID id, const ast::RangeExpression& range) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(range.lhs);
    auto& lhs_type{*last_type_.take()};
    TRY_RESOLVE(range.rhs);

    // Due to deferred type checking just use one type
    auto& slice_type{ctx_.get_slice(types::mut::CONSTANT, false, lhs_type)};
    resolving_.set_sema_type(id, slice_type);
    last_type_.emplace(slice_type);
}

namespace {

[[nodiscard]] constexpr auto plurality(const auto& vec) noexcept -> std::string_view {
    return vec.size() == 1 ? "" : "s";
}

} // namespace

auto TypeResolver::validate_struct_initializer(ast::NodeID                       init_id,
                                               const ast::InitializerExpression& init,
                                               Type& struct_type) -> Result<void, Diagnostic> {
    struct_validator_.clear();
    const auto& struct_data{struct_type.get_data().as<types::Struct>()};

    // Check for duplicates
    for (const auto& [accessor, value] : init.initializers) {
        const auto  accessor_node{resolving_.ast.get_as<ast::ImplicitAccessExpression>(accessor)};
        const auto& accessor_ident =
            resolving_.ast.get_as<ast::IdentifierExpression>(accessor_node.member);
        const auto field_name{accessor_ident.name};
        if (!struct_validator_.seen.insert(field_name).second) {
            struct_validator_.duplicates.emplace_back(field_name);
        }
        struct_validator_.provided.emplace_back(field_name);
    }

    if (!struct_validator_.duplicates.empty()) {
        return Diagnostic{fmt::format("Struct initializer contains duplicate field{}: {}",
                                      plurality(struct_validator_.duplicates),
                                      fmt::join(struct_validator_.duplicates, ", ")),
                          Error::DUPLICATE_FIELD,
                          resolving_.ast.location_of(init_id)};
    }

    // Check for missing fields
    const auto& enclosing{struct_data.enclosing};
    for (const auto& [ident, _, default_value] : struct_data.ast_fields) {
        const auto& field_node{enclosing.ast.get_as<ast::IdentifierExpression>(ident)};
        if (!default_value && !struct_validator_.seen.contains(field_node.name)) {
            struct_validator_.missings.emplace_back(field_node.name);
        }
    }

    if (!struct_validator_.missings.empty()) {
        return Diagnostic{fmt::format("Struct initializer missing required field{}: {}",
                                      plurality(struct_validator_.missings),
                                      fmt::join(struct_validator_.missings, ", ")),
                          Error::MISSING_FIELD,
                          resolving_.ast.location_of(init_id)};
    }

    // Check for extra/unknown fields
    const auto  struct_table_idx{struct_type.get_symbol_table_idx()};
    const auto& struct_table{ctx_.registry.get(struct_table_idx)};
    for (const auto name : struct_validator_.provided) {
        if (!struct_table.has(name)) { struct_validator_.unknowns.emplace_back(name); }
    }

    if (!struct_validator_.unknowns.empty()) {
        return Diagnostic{fmt::format("Struct initializer contains unknown field{}: {}",
                                      plurality(struct_validator_.unknowns),
                                      fmt::join(struct_validator_.unknowns, ", ")),
                          Error::UNKNOWN_FIELD,
                          resolving_.ast.location_of(init_id)};
    }
    return {};
}

auto TypeResolver::visit(ast::NodeID id, const ast::InitializerExpression& init) -> void {
    PROFILE_FUNCTION();

    // Resolve the object first so it can be tied to the member's types
    opt::Option<Type&> object_type_opt;
    if (init.object_type) {
        TRY_RESOLVE(*init.object_type);
        object_type_opt.emplace(*last_type_.take());
    } else if (const auto implicit_type{implicit_type_stack_.peek()}) {
        object_type_opt.emplace(*implicit_type);
    } else {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             "Initializer expression requires a known type; provide an explicit "
                             "type or use in a typed context",
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }

    Type& object_type{*object_type_opt};
    if (!object_type.is_resolved()) {
        return last_type_.emplace(ctx_.poison_node(resolving_,
                                                   id,
                                                   "Cannot initialize an incomplete type",
                                                   Error::CYCLIC_DEPENDENCY,
                                                   resolving_.ast.location_of(id)));
    }

    const auto  num_initializers{init.initializers.size()};
    const auto& object_data{object_type.get_data()};
    if (object_data.is<types::Enum>()) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             "Enums cannot be initialized with an initializer expression as "
                             "they lack member variables",
                             Error::ARITY_MISMATCH,
                             resolving_.ast.location_of(id)));
    }
    if (object_data.is<types::Union>()) {
        // This is a restriction naturally imposed by the definition of a union in theory
        if (num_initializers != 1) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format("Union initializer lists must list exactly one field; found {}",
                            num_initializers),
                Error::ARITY_MISMATCH,
                resolving_.ast.location_of(id)));
        }
    } else if (const auto struct_data{object_data.as_opt<types::Struct>()}) {
        if (auto valid{validate_struct_initializer(id, init, object_type)}; !valid) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(valid).error()));
        }
    }

    if (!object_data.is<types::Struct>() && !object_data.is<types::Union>()) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Only struct and union types may be used in "
                                         "initializer expressions; found '{}'",
                                         type_kind_display_name(object_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }

    for (const auto& [accessor, value] : init.initializers) {
        // The accessor is always a lookup into the stack
        {
            const StructuralGuard g{implicit_type_stack_, object_type};
            TRY_RESOLVE(accessor);
        }

        // The value might be a necessary implicit access
        auto& member_type{resolving_.get_sema_type(accessor)};
        if (member_type.is_poison()) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id));
        }

        {
            const StructuralGuard g{implicit_type_stack_, member_type};
            TRY_RESOLVE(value);
        }
    }

    resolving_.set_sema_type(id, object_type);
    last_type_.emplace(object_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::LabelExpression& label) -> void {
    PROFILE_FUNCTION();
    auto&       label_type{resolving_.get_sema_type(id)};
    const Scope s{table_stack_, label_type.get_symbol_table_idx(), table_idx_};

    // Resolve the body but cache the label's type so the result can bind to the label
    TRY_RESOLVE(*label.body);
    auto symbol{resolve_symbol_info(label.name, opt::none)};
    if (!symbol) { return; }
    auto& label_data{symbols::Label::from(*symbol)};

    // Labels may not be used outside of continues which don't push void
    if (!label_data.has_yield_types()) {
        label_data.add_yield_type(ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }

    // The last type inherits the result type to help propagation of poison
    auto& result_type{*label_data.get_yield_types()[0]};
    ASSERT(result_type.is_resolved(), "The label's inner type should've been resolved");
    label_type.resolve_if<Type::Data>(result_type.get_data());
    resolving_.set_sema_type(label.name, result_type);
    last_type_.emplace(result_type);
}

namespace {

// Collects potential duplicate implicit access match arms for the structural type
auto gather_arm_duplicates(gsl::span<const ast::MatchExpression::Arm> arms,
                           mod::Module&                               resolving,
                           TypeResolver::StructuralValidator&         validator,
                           bool require_implicit_access) -> opt::Option<Diagnostic> {
    for (const auto& arm : arms) {
        if (arm.pattern.is<ast::Discarded>()) { continue; }

        // It's only possible to verify access expressions
        const auto pattern_node{
            resolving.ast.get_as_opt<ast::ImplicitAccessExpression>(arm.pattern)};
        if (!pattern_node) {
            if (require_implicit_access) {
                return Diagnostic{
                    "Match arm may only have an implicit access pattern in this context",
                    Error::ILLEGAL_MATCH_PATTERN,
                    resolving.ast.location_of(arm.pattern)};
            }
            continue;
        }

        const auto& ident{resolving.ast.get_as<ast::IdentifierExpression>(pattern_node->member)};
        if (!validator.seen.insert(ident.name).second) {
            validator.duplicates.emplace_back(ident.name);
        }
        validator.provided.emplace_back(ident.name);
    }
    return opt::none;
}

} // namespace

auto TypeResolver::validate_enum_arms(ast::NodeID                 match_id,
                                      const ast::MatchExpression& match,
                                      Type& enum_type) -> opt::Option<Diagnostic> {
    enum_validator_.clear();

    if (enum_type.get_data().as<types::Enum>().non_exhaustive && !match.catch_all_idx) {
        return Diagnostic{"Match expressions over non-exhaustive enums must have a catch all arm",
                          Error::ILLEGAL_MATCH_PATTERN,
                          resolving_.ast.location_of(match_id)};
    }

    // Track seen and duplicate variants in the match arms
    const auto diag{gather_arm_duplicates(match.arms, resolving_, enum_validator_, false)};
    ASSERT(!diag, "Enum validation should not return a diagnostic");
    if (!enum_validator_.duplicates.empty()) {
        return Diagnostic{fmt::format("Match expression contains duplicate enumeration{}: {}",
                                      plurality(enum_validator_.duplicates),
                                      fmt::join(enum_validator_.duplicates, ", ")),
                          Error::DUPLICATE_ENUMERATION,
                          resolving_.ast.location_of(match_id)};
    }

    // Check for unknown/extra variants
    const auto  enum_table_idx{enum_type.get_symbol_table_idx()};
    const auto& enum_table{ctx_.registry.get(enum_table_idx)};
    for (const auto name : enum_validator_.provided) {
        if (!enum_table.has(name)) { enum_validator_.unknowns.emplace_back(name); }
    }

    if (!enum_validator_.unknowns.empty()) {
        return Diagnostic{fmt::format("Match expression contains unknown enumeration{}: {}",
                                      plurality(enum_validator_.unknowns),
                                      fmt::join(enum_validator_.unknowns, ", ")),
                          Error::UNKNOWN_ENUMERATION,
                          resolving_.ast.location_of(match_id)};
    }
    return opt::none;
}

auto TypeResolver::validate_union_arms(ast::NodeID                 match_id,
                                       const ast::MatchExpression& match,
                                       Type& union_type) -> opt::Option<Diagnostic> {
    union_validator_.clear();
    const auto& union_data{union_type.get_data().as<types::Union>()};

    // Track seen and duplicate fields in the match arms
    if (auto diag{gather_arm_duplicates(match.arms, resolving_, union_validator_, true)}; diag) {
        return diag;
    }

    if (!union_validator_.duplicates.empty()) {
        return Diagnostic{fmt::format("Match expression contains duplicate union field{}: {}",
                                      plurality(union_validator_.duplicates),
                                      fmt::join(union_validator_.duplicates, ", ")),
                          Error::DUPLICATE_FIELD,
                          resolving_.ast.location_of(match_id)};
    }

    // Check for missing fields only if there's a missing catch all
    if (!match.catch_all_idx) {
        const auto& enclosing{union_data.enclosing};
        for (const auto& [ident, _] : union_data.ast_fields) {
            const auto& field_node{enclosing.ast.get_as<ast::IdentifierExpression>(ident)};
            if (!union_validator_.seen.contains(field_node.name)) {
                union_validator_.missings.emplace_back(field_node.name);
            }
        }

        if (!union_validator_.missings.empty()) {
            return Diagnostic{
                fmt::format("Match expression is non-exhaustive; missing union field{}: {}",
                            plurality(union_validator_.missings),
                            fmt::join(union_validator_.missings, ", ")),
                Error::MISSING_FIELD,
                resolving_.ast.location_of(match_id)};
        }
    }

    // Check for unknown fields
    const auto  union_table_idx{union_type.get_symbol_table_idx()};
    const auto& union_table{ctx_.registry.get(union_table_idx)};
    for (const auto name : union_validator_.provided) {
        if (!union_table.has(name)) { union_validator_.unknowns.emplace_back(name); }
    }

    if (!union_validator_.unknowns.empty()) {
        return Diagnostic{fmt::format("Match expression contains unknown union field{}: {}",
                                      plurality(union_validator_.unknowns),
                                      fmt::join(union_validator_.unknowns, ", ")),
                          Error::UNKNOWN_FIELD,
                          resolving_.ast.location_of(match_id)};
    }
    return opt::none;
}

auto TypeResolver::visit(ast::NodeID id, const ast::MatchExpression& match) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(match.matcher);
    auto&                 matcher_type{*last_type_.take()};
    const StructuralGuard g{implicit_type_stack_, matcher_type};

    // The expression must resolve to a single type on pass 3
    opt::Option<Type&> first_type;
    const auto         try_set_first_type = [&] -> void {
        if (!first_type) {
            first_type = last_type_.take();
        } else {
            last_type_.reset();
        }
    };

    // Rip through the arms once to validate structural arm rules
    const auto& matcher_data{matcher_type.get_data()};
    if (matcher_data.is<types::Enum>()) {
        if (auto diag{validate_enum_arms(id, match, matcher_type)}; diag) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(diag).value()));
        }
    } else if (matcher_data.is<types::Union>()) {
        if (auto diag{validate_union_arms(id, match, matcher_type)}; diag) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(diag).value()));
        }
    } else if (matcher_data.is<types::BuiltinType>()) {
        // It's assumed that any sufficiently large type cannot be fully enumerated
        opt::Option<u16> required_arm_count;
        switch (matcher_type.get_kind()) {
        case TypeKind::I32:
        case TypeKind::I64:
        case TypeKind::ISIZE:
        case TypeKind::U32:
        case TypeKind::U64:
        case TypeKind::USIZE: break;
        case TypeKind::U8:    required_arm_count.emplace(256); break;
        case TypeKind::BOOL:  required_arm_count.emplace(2); break;
        case TypeKind::F32:
        case TypeKind::F64:
            return last_type_.emplace(
                ctx_.poison_node(resolving_,
                                 id,
                                 "Cannot match on floats due to precision; use an if statement "
                                 "with explicit precision handling",
                                 sema::Error::TYPE_MISMATCH,
                                 resolving_.ast.location_of(match.matcher)));
        case TypeKind::VOID:
        case TypeKind::UNDEFINED:
            return last_type_.emplace(ctx_.poison_node(resolving_,
                                                       id,
                                                       "Empty types cannot be matched on",
                                                       sema::Error::TYPE_MISMATCH,
                                                       resolving_.ast.location_of(match.matcher)));
        case TypeKind::TYPE:
        case TypeKind::AUTO:
        case TypeKind::OPAQUE:
        case TypeKind::NORETURN:
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format("Can only match on integers, bytes, and booleans; found '{}'",
                            type_kind_display_name(matcher_type.get_kind())),
                sema::Error::TYPE_MISMATCH,
                resolving_.ast.location_of(match.matcher)));
        default: UNREACHABLE("Builtin types should never take this type kind");
        }

        if (!match.catch_all_idx) {
            // With a required arm count the counts must match
            if (required_arm_count && required_arm_count != match.arms.size()) {
                return last_type_.emplace(ctx_.poison_node(
                    resolving_,
                    id,
                    fmt::format("Matching on type '{}' requires a catch all arm with "
                                "a pattern of '_' or exactly {} patterned arms",
                                type_kind_display_name(matcher_type.get_kind()),
                                *required_arm_count),
                    sema::Error::TYPE_MISMATCH,
                    resolving_.ast.location_of(match.matcher)));
            }

            if (!required_arm_count) {
                // Otherwise there must always be a catch all arm
                return last_type_.emplace(ctx_.poison_node(
                    resolving_,
                    id,
                    fmt::format(
                        "Matching on type '{}' requires a catch all arm with a pattern of '_'",
                        type_kind_display_name(matcher_type.get_kind())),
                    sema::Error::TYPE_MISMATCH,
                    resolving_.ast.location_of(match.matcher)));
            }
        }
    } else {
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            fmt::format("Can only match on enums, unions, and certain primitive types; found '{}'",
                        type_kind_display_name(matcher_type.get_kind())),
            sema::Error::TYPE_MISMATCH,
            resolving_.ast.location_of(match.matcher)));
    }

    // Each arm was assigned a new scope index on the first pass
    for (const auto& arm : match.arms) {
        // Tabled types have prefilled types that should be pushed on the table stack
        auto&       arm_type{resolving_.get_sema_type(arm)};
        const Scope scope{table_stack_, arm_type.get_symbol_table_idx(), table_idx_};

        if (arm.capture && arm.capture->is<ast::IdentifierExpression>()) {
            // Unions implicitly unpack the value since the field is guaranteed to be valid
            if (const auto union_data{matcher_data.as_opt<types::Union>()}) {
                // At this point the pattern is guaranteed to be an implicit access
                const auto implicit_access{
                    resolving_.ast.get_as_opt<ast::ImplicitAccessExpression>(arm.pattern)};
                ASSERT(implicit_access, "Union validator failed to error");
                const auto& ident =
                    resolving_.ast.get_as<ast::IdentifierExpression>(implicit_access->member);

                const auto& table{ctx_.registry.get(matcher_type.get_symbol_table_idx())};
                const auto& proxy{table.get_proxy(ident.name)};
                resolving_.set_sema_type(*arm.capture, union_data->type_at(proxy.index));
            } else {
                resolving_.set_sema_type(*arm.capture, matcher_type);
            }

            resolve_symbol_info(*arm.capture, SymbolKind::VALUE);
        }

        if (!arm.pattern.is<ast::Discarded>()) { TRY_RESOLVE(arm.pattern); }
        TRY_RESOLVE(arm.dispatch);

        // Set the arms type to the dispatch only if its not occupied by a tabled type
        try_set_first_type();
    }

    // In the rare case that a type could not be found we have to poison
    if (first_type) {
        resolving_.set_sema_type(id, *first_type);
        last_type_.emplace(*first_type);
    } else {
        last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
}

namespace {

// Returns the mutability associated with the reference/address-of operator
[[nodiscard]] auto ref_addr_of_is_mutable(ast::NodeID id) noexcept -> types::MutabilityModifiers {
    using syntax::TokenType;
    switch (id.get_token_type()) {
    case TokenType::BW_AND:    return types::mut::CONSTANT;
    case TokenType::AND_MUT:   return types::mut::MUTABLE;
    case TokenType::CARET:     return types::mut::CONSTANT;
    case TokenType::CARET_MUT: return types::mut::MUTABLE;
    default:                   UNREACHABLE("Invalid token types should be pruned prior to this function");
    }
}

} // namespace

auto TypeResolver::visit(ast::NodeID id, const ast::ReferenceExpression& ref) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(ref.rhs);
    auto& rhs_type{*last_type_.take()};

    auto& new_type{ctx_.get_reference(ref_addr_of_is_mutable(id), rhs_type)};
    new_type.resolve<types::Reference>(rhs_type);

    resolving_.set_sema_type(id, new_type);
    last_type_.emplace(new_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::AddressOfExpression& adr_of) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(adr_of.rhs);
    auto& rhs_type{*last_type_.take()};

    auto& new_type{ctx_.get_pointer(ref_addr_of_is_mutable(id), rhs_type)};
    new_type.resolve_if<types::Pointer>(rhs_type);

    resolving_.set_sema_type(id, new_type);
    last_type_.emplace(new_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::DereferenceExpression& deref) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(deref.rhs);
    auto& rhs_type{*last_type_.take()};

    // Check for a pointer and update to the underlying type to enforce dereference semantics
    if (const auto pointer{rhs_type.get_data().as_opt<types::Pointer>()}) {
        last_type_.emplace(pointer->underlying);
    } else {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Cannot dereference non-pointer expression; found '{}'",
                                         type_kind_display_name(rhs_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::UnaryExpression& node) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(node.rhs);
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::ImplicitAccessExpression& implicit_access)
    -> void {
    PROFILE_FUNCTION();
    const auto implicit_type{implicit_type_stack_.peek()};
    if (!implicit_type) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             "Implicit access expression used outside of a typed context",
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }

    auto result{resolve_structural_access(
        *implicit_type, implicit_access.member, resolving_.ast.location_of(id))};
    if (!result) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result).error()));
    }

    auto& member_type{*result.value()};
    resolving_.set_sema_type(implicit_access.member, member_type);
    resolving_.set_sema_type(id, member_type);
    last_type_.emplace(member_type);
}

// String literals are just constant arrays of bytes
auto TypeResolver::visit(ast::NodeID id, const ast::StringExpression& string) -> void {
    PROFILE_FUNCTION();

    // String literals are null terminated since they can be trivially shortened to non null
    auto& type{ctx_.get_array(types::mut::CONSTANT,
                              true,
                              string.value.size() + 1,
                              ctx_.get_builtin_resolved_type(TypeKind::U8))};

    // String literals with the same size will always have the same type
    resolving_.set_sema_type(id, type);
    last_type_.emplace(type);
}

#define MAKE_PRIMITIVE_RESOLVER(NodeType, kind)                              \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType&) -> void { \
        PROFILE_FUNCTION();                                                  \
        last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::kind));  \
        last_type_->resolve_if<types::BuiltinType>();                        \
        resolving_.set_sema_type(id, *last_type_);                           \
    }

MAKE_PRIMITIVE_RESOLVER(I32Expression, I32)
MAKE_PRIMITIVE_RESOLVER(I64Expression, I64)
MAKE_PRIMITIVE_RESOLVER(ISizeExpression, ISIZE)
MAKE_PRIMITIVE_RESOLVER(U32Expression, U32)
MAKE_PRIMITIVE_RESOLVER(U64Expression, U64)
MAKE_PRIMITIVE_RESOLVER(USizeExpression, USIZE)
MAKE_PRIMITIVE_RESOLVER(U8Expression, U8)
MAKE_PRIMITIVE_RESOLVER(BoolExpression, BOOL)
MAKE_PRIMITIVE_RESOLVER(VoidExpression, VOID)
MAKE_PRIMITIVE_RESOLVER(UndefinedExpression, UNDEFINED)
MAKE_PRIMITIVE_RESOLVER(F32Expression, F32)
MAKE_PRIMITIVE_RESOLVER(F64Expression, F64)

template <traits::IndexableID ID>
auto TypeResolver::resolve_module_access(ID id, const ast::ModuleAccessExpression& access) -> void {
    // Resolving the right hand side recurses down to the identifier level
    resolve(access.outer);
    if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); }
    auto& outer_type{*last_type_.take()};
    auto& outer_resolved{outer_type.get_data()};

    if (const auto module{outer_resolved.as_opt<types::Module>()}) {
        // The module may not have been resolved yet due to order independence
        auto& inner_mod{module->imported};
        if (inner_mod.is_resolvable()) {
            Context new_ctx{ctx_};
            resolve_types(inner_mod, new_ctx);
            if (inner_mod.is_poisoned()) {
                return last_type_.emplace(ctx_.poison_node(resolving_, id));
            }
        }

        // Step into the module's scope for lookup
        const auto& inner_ident{resolving_.ast.get_as<ast::IdentifierExpression>(access.inner)};
        auto        symbol{ctx_.registry.get_from_opt(*inner_mod.root_table_idx, inner_ident.name)};
        if (!symbol) {
            return last_type_.emplace(
                ctx_.poison_node(resolving_,
                                 id,
                                 fmt::format("Module '{}' has no member named '{}'",
                                             get_rightmost_name(access.outer),
                                             inner_ident.name),
                                 Error::UNDECLARED_IDENTIFIER,
                                 resolving_.ast.location_of(access.inner)));
        }

        const auto symbol_node{symbol->get_data().as_opt<symbols::Node>()};
        if (!symbol_node) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        opt::Option<ast::TypeModifier> mod;
        if constexpr (traits::ExplicitTypeID<ID>) { mod = id.get_modifier(); }
        switch (symbol->get_status()) {
        case SymbolStatus::RESOLVING: {
            const auto poison_out = [&] -> void {
                ctx_.poison_symbol(*symbol);
                last_type_.emplace(ctx_.poison_node(
                    resolving_,
                    id,
                    fmt::format(
                        "Cross-module cyclic dependency detected while resolving symbol '{}'",
                        inner_ident.name),
                    Error::CYCLIC_DEPENDENCY,
                    resolving_.ast.location_of(access.inner)));
            };

            // Explicitly reject infinite size cycles across modules before forwarding
            if (!mod || (!mod->is_ptr() && !mod->is_ref())) { return poison_out(); }
            if (const auto forwarded_type{forward_type(inner_mod, mod, *symbol)}) {
                resolving_.set_sema_type(access.inner, *forwarded_type);
                resolving_.set_sema_type(id, *forwarded_type);
                return last_type_.emplace(*forwarded_type);
            }

            return poison_out();
        }
        case SymbolStatus::UNRESOLVED: {
            TypeResolver inner_resolver{inner_mod, ctx_};
            inner_resolver.resolve(*symbol_node);
            break;
        }
        case SymbolStatus::RESOLVED: break;
        }

        if (symbol->get_kind() == SymbolKind::POISONED || !inner_mod.has_sema_type(*symbol_node)) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id));
        }

        auto& ident_type{inner_mod.get_sema_type(*symbol_node)};
        resolving_.set_sema_type(access.inner, ident_type);
        resolving_.set_sema_type(id, ident_type);
        return last_type_.emplace(ident_type);
    }

    if (outer_resolved.is<types::Struct>() || outer_resolved.is<types::Enum>() ||
        outer_resolved.is<types::Union>()) {
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            fmt::format("Use the dot operator '.' to access {} fields; found module access '::'",
                        type_kind_display_name(outer_type.get_kind())),
            Error::TYPE_MISMATCH,
            resolving_.ast.location_of(access.outer)));
    }

    return last_type_.emplace(ctx_.poison_node(
        resolving_,
        id,
        fmt::format("Module access operator '::' can only be applied to modules; found '{}'",
                    type_kind_display_name(outer_type.get_kind())),
        Error::TYPE_MISMATCH,
        resolving_.ast.location_of(access.outer)));
}

VISITOR_TEMPLATE_INIT(TypeResolver, resolve_module_access, const ast::ModuleAccessExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::ModuleAccessExpression& scope) -> void {
    PROFILE_FUNCTION();
    resolve_module_access(id, scope);
}

namespace {

[[nodiscard]] auto incomplete_field(std::string_view name, const SourceLocation& location)
    -> Diagnostic {
    return Diagnostic{
        fmt::format("Field '{}' has an incomplete type; creates an infinite size cycle", name),
        Error::CYCLIC_DEPENDENCY,
        location};
}

} // namespace

template <traits::IndexableID ID>
auto TypeResolver::visit(ID id, const ast::StructExpression& struct_expr) -> void {
    PROFILE_FUNCTION();
    auto&                 struct_type{resolving_.get_sema_type(id)};
    const Scope           s{table_stack_, struct_type.get_symbol_table_idx(), table_idx_};
    const StructuralGuard g{user_type_stack_, struct_type};

    auto field_types{ctx_.pool.get_many_unsafe(struct_expr.fields.size())};
    for (usize i{0}; const auto& field : struct_expr.fields) {
        const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(field.name)};
        auto        symbol{ctx_.registry.get_from_opt(table_idx_, ident.name)};
        if (!symbol) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        TRY_RESOLVE(field.explicit_type);
        auto& field_type{*last_type_.take()};

        if (field.default_value) {
            const StructuralGuard inner_g{implicit_type_stack_, field_type};
            TRY_RESOLVE(*field.default_value);
        }

        if (!field_type.is_resolved()) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                incomplete_field(ident.name, resolving_.ast.location_of(field.explicit_type))));
        }

        resolving_.set_sema_type(field.name, field_type);
        symbol->set_kind(SymbolKind::VALUE);
        symbol->set_status(SymbolStatus::RESOLVED);
        field_types[i++] = &field_type;
    }

    auto member_types{ctx_.pool.get_many_unsafe(struct_expr.members.size())};
    CommittableResolution<types::Struct> resolution{
        struct_type, field_types, struct_expr.fields, member_types, resolving_};
    if (!resolve_members(member_types, struct_expr.members)) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }

    resolution.commit();
    last_type_.emplace(struct_type);
}

VISITOR_TEMPLATE_INIT(TypeResolver, visit, const ast::StructExpression&)

template <traits::IndexableID ID>
auto TypeResolver::visit(ID id, const ast::UnionExpression& union_expr) -> void {
    PROFILE_FUNCTION();
    auto&                 union_type{resolving_.get_sema_type(id)};
    const Scope           s{table_stack_, union_type.get_symbol_table_idx(), table_idx_};
    const StructuralGuard g{user_type_stack_, union_type};

    auto field_types{ctx_.pool.get_many_unsafe(union_expr.fields.size())};
    for (usize i{0}; const auto& field : union_expr.fields) {
        const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(field.name)};
        auto        symbol{ctx_.registry.get_from_opt(table_idx_, ident.name)};
        if (!symbol) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        TRY_RESOLVE(field.explicit_type);
        auto& field_type{*last_type_.take()};

        if (!field_type.is_resolved()) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                incomplete_field(ident.name, resolving_.ast.location_of(field.explicit_type))));
        }

        resolving_.set_sema_type(field.name, field_type);
        symbol->set_kind(SymbolKind::VALUE);
        symbol->set_status(SymbolStatus::RESOLVED);
        field_types[i++] = &field_type;
    }

    auto member_types{ctx_.pool.get_many_unsafe(union_expr.members.size())};
    CommittableResolution<types::Union> resolution{
        union_type, field_types, union_expr.fields, member_types, resolving_};
    if (!resolve_members(member_types, union_expr.members)) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }

    resolution.commit();
    last_type_.emplace(union_type);
}

VISITOR_TEMPLATE_INIT(TypeResolver, visit, const ast::UnionExpression&)

auto TypeResolver::visit(ast::NodeID id, const ast::WhileLoopExpression& while_loop) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(while_loop.condition);
    if (while_loop.continuation) { TRY_RESOLVE(*while_loop.continuation); }
    // The loop itself holds the block index which houses captures, not the block
    auto& loop_type{resolving_.get_sema_type(id)};
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};
        const auto& block{resolving_.ast.get_as<ast::BlockStatement>(while_loop.block)};
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    if (while_loop.non_break) { TRY_RESOLVE(*while_loop.non_break); }
    last_type_.emplace(loop_type);
}

// DONT CALL ME FROM ANY LOOP/CONDITION/FN RESOLVER
auto TypeResolver::visit(ast::NodeID id, const ast::BlockStatement& block) -> void {
    PROFILE_FUNCTION();
    auto&       block_type{resolving_.get_sema_type(id)};
    const Scope s{table_stack_, block_type.get_symbol_table_idx(), table_idx_};

    // Just an abridged loop handler
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(block_type);
}

auto TypeResolver::resolve_control_flow_label(opt::Option<ast::IdentifierHandle> label,
                                              std::string_view                   stmt_name)
    -> Result<opt::Option<Symbol&>, Diagnostic> {
    if (label) {
        const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(*label)};
        auto        symbol{ctx_.registry.lookup(table_stack_, ident.name)};
        if (!symbol || symbol->get_kind() != SymbolKind::LABEL) {
            return make_sema_err(
                fmt::format("Labeled {} statements must be used with a known label", stmt_name),
                Error::ILLEGAL_CONTROL_FLOW,
                resolving_.ast.location_of(*label));
        }
        return symbol;
    }
    return opt::none;
}

auto TypeResolver::visit(ast::NodeID id, const ast::BreakStatement& break_stmt) -> void {
    PROFILE_FUNCTION();
    auto result{resolve_control_flow_label(break_stmt.label, "break")};
    if (!result) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result).error()));
    }
    auto symbol{result.value()};

    // No payload is semantically equivalent to breaking with void
    if (symbol) {
        auto& label_data{symbols::Label::from(*symbol)};

        if (break_stmt.expression) {
            TRY_RESOLVE(*break_stmt.expression);
            auto& expression_type{*last_type_.take()};
            label_data.add_yield_type(expression_type);
            resolving_.set_sema_type(id, expression_type);
        } else {
            auto& void_type{ctx_.get_builtin_resolved_type(TypeKind::VOID)};
            label_data.add_yield_type(void_type);
            resolving_.set_sema_type(id, void_type);
        }
    } else {
        resolving_.set_sema_type(id, ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }

    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ContinueStatement& continue_stmt) -> void {
    PROFILE_FUNCTION();
    auto result{resolve_control_flow_label(continue_stmt.label, "continue")};
    if (!result) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result).error()));
    }

    resolving_.set_sema_type(id, ctx_.get_builtin_resolved_type(TypeKind::VOID));
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DeclStatement& decl) -> void {
    PROFILE_FUNCTION();
    const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(*decl.name)};
    auto        symbol_opt{ctx_.registry.lookup(table_stack_, ident.name)};
    ASSERT(symbol_opt, "Somehow the declaration was lost in the symbol table");
    auto& symbol{*symbol_opt};

    // Breaking out early is possible due to out of order semantics
    if (symbol.get_status() == SymbolStatus::RESOLVED) {
        ASSERT(resolving_.has_sema_type(id), "Resolved decl has no type");
        return last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }
    symbol.set_status(SymbolStatus::RESOLVING);

    const auto poison_out = [&] -> void {
        resolving_.set_sema_type(decl.name, *last_type_);
        ctx_.poison_symbol(symbol);
        resolving_.set_sema_type(id, *last_type_);
    };

    {
        opt::Option<StructuralGuard> type_guard;

        // With an explicit type, the ident should always adopt that exact type
        if (decl.explicit_type) {
            resolve(*decl.explicit_type);
            if (last_type_->is_poison()) { return poison_out(); }
            auto& explicit_type{*last_type_.take()};
            type_guard.emplace(implicit_type_stack_, explicit_type);
            resolving_.set_sema_type(id, explicit_type);
        }

        // Only update the decl value type if it hasn't been set
        if (decl.value) {
            resolve(*decl.value);
            if (last_type_->is_poison()) { return poison_out(); }
            resolving_.set_sema_type_if(id, *last_type_.take());
        }
    }

    // The symbol's kind can be fully resolved with knowledge of the declarations type
    auto&       resolved_type{resolving_.get_sema_type(id)};
    const auto& type_data{resolved_type.get_data()};
    if (resolved_type.is_poison()) {
        symbol.set_kind(SymbolKind::POISONED);
    } else if (!symbol.has_kind()) {
        if (type_data.is<types::BuiltinFunction>() || type_data.is<types::Function>()) {
            symbol.set_kind(SymbolKind::CALLABLE);
        } else if (type_data.is<types::Enum>() || type_data.is<types::Struct>() ||
                   type_data.is<types::Union>() ||
                   resolved_type == ctx_.get_builtin_resolved_type(TypeKind::TYPE)) {
            symbol.set_kind(SymbolKind::TYPE);
        } else if (type_data.is<types::Module>()) {
            symbol.set_kind(SymbolKind::MODULE);
        } else {
            symbol.set_kind(SymbolKind::VALUE);
        }
    }

    resolving_.set_sema_type_if(decl.name, resolved_type);
    symbol.set_status(SymbolStatus::RESOLVED);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DeferStatement& defer) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(defer.deferred);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DiscardStatement& discard) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(discard.discarded);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ExpressionStatement& expr) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(expr.expression);
    resolving_.set_sema_type(expr.expression, *last_type_.take());
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ImportStatement& import_stmt) -> void {
    PROFILE_FUNCTION();
    const auto [ident_id, name]{import_stmt.get_name(resolving_.ast)};
    auto& symbol{ctx_.registry.get_from(table_idx_, name)};

    const auto poison_out = [&] -> void {
        last_type_.emplace(ctx_.get_poison());
        resolving_.set_sema_type(ident_id, *last_type_);
        ctx_.poison_symbol(symbol);
    };

    // Updating this type reflects on the symbol in the actual table as well
    auto& import_type{resolving_.get_sema_type(id)};
    if (import_type.is_poison()) { return poison_out(); }
    ASSERT(import_type.is_resolved(), "Import types should be resolved on pass 1");
    auto& module{import_type.get_data().as<types::Module>()};

    // There's no need to poison the import type since it would lose all of the module information
    Context new_ctx{ctx_};
    resolve_types(module.imported, new_ctx);
    if (module.imported.is_poisoned()) { return poison_out(); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ReturnStatement& return_stmt) -> void {
    PROFILE_FUNCTION();
    if (return_stmt.expression) { TRY_RESOLVE(*return_stmt.expression); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::TestStatement& test) -> void {
    PROFILE_FUNCTION();
    auto&       test_type{resolving_.get_sema_type(id)};
    const Scope s{table_stack_, test_type.get_symbol_table_idx(), table_idx_};

    // Duplicate test blocks are a big no-no if named
    if (test.description) {
        const auto& description{resolving_.ast.get_as<ast::StringExpression>(*test.description)};
        const auto& name{description.value};
        auto [it, inserted]{named_tests_.try_emplace(name, id)};
        if (!inserted) {
            const auto original_loc{resolving_.ast.location_of(it->second)};
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format("Duplicate test block named '{}'; previous declaration here: {}",
                            name,
                            original_loc),
                Error::DUPLICATE_TEST_NAME,
                resolving_.ast.location_of(id)));
        }
    }

    const auto& block{resolving_.ast.get_as<ast::BlockStatement>(test.block)};
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::UsingStatement& using_stmt) -> void {
    PROFILE_FUNCTION();
    const auto& ident{resolving_.ast.get_as<ast::IdentifierExpression>(using_stmt.alias)};
    auto        symbol{ctx_.registry.get_from_opt(table_idx_, ident.name)};
    if (!symbol) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }
    if (symbol->get_status() == SymbolStatus::RESOLVED) {
        ASSERT(resolving_.has_sema_type(id), "Resolved alias has no type");
        return last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }

    const auto poison_out = [&] -> void {
        resolving_.set_sema_type(using_stmt.alias, *last_type_);
        resolving_.set_sema_type(id, *last_type_);
        ctx_.poison_symbol(*symbol);
    };

    // Bind the resolved type to the symbol now that its been collected
    symbol->set_status(SymbolStatus::RESOLVING);
    resolve(using_stmt.explicit_type);
    if (last_type_->is_poison()) { return poison_out(); }
    resolving_.set_sema_type(id, *last_type_.take());

    symbol->set_status(SymbolStatus::RESOLVED);
    resolve(using_stmt.alias);
    if (last_type_->is_poison()) { return poison_out(); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

// Without a modifier or with poison the result should be the same as the node
auto TypeResolver::apply_explicit_modifiers(ast::ExplicitTypeID id, Type& inner_type) -> Type& {
    const auto modifier{id.get_modifier()};
    if (modifier.is_value() || inner_type.is_poison()) { return inner_type; }
    const auto mutability{mutability_from_type_modifier(modifier)};

    // Conditionally update the mutability since the modifier might entail mutability or volatility
    auto new_key{inner_type.get_key()};
    if (mutability) { new_key.set_mut(*mutability); }

    // The key should reflect the new kind and should not double imprint the same type
    if (modifier.is_ptr()) {
        new_key.clear_markers();
        new_key.set_kind(TypeKind::POINTER);
        new_key.imprint(inner_type);

        auto& new_ptr_type{*ctx_.pool[new_key]};
        new_ptr_type.resolve_if<types::Pointer>(inner_type);
        return new_ptr_type;
    }

    if (modifier.is_ref()) {
        new_key.clear_markers();
        new_key.set_kind(TypeKind::REFERENCE);
        new_key.imprint(inner_type);

        auto& new_ref_type{*ctx_.pool[new_key]};
        new_ref_type.resolve_if<types::Reference>(inner_type);
        return new_ref_type;
    }

    if (modifier.is_volatile()) {
        // Volatility is baked into mutability and should not be imprinted
        auto& new_vol_type{*ctx_.pool[new_key]};
        new_vol_type.resolve_if<Type::Data>(inner_type.get_data());
        return new_vol_type;
    }
    UNREACHABLE("A new type modifier was likely added yet unaccounted for");
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::IdentifierExpression& ident) -> void {
    PROFILE_FUNCTION();
    auto symbol_opt{ctx_.registry.lookup(table_stack_, ident.name)};
    if (!symbol_opt) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Use of undeclared identifier '{}'", ident.name),
                             Error::UNDECLARED_IDENTIFIER,
                             resolving_.ast.location_of(id)));
    }
    auto& symbol{*symbol_opt};

    const auto forwarded_type{forward_type(resolving_, id.get_modifier(), symbol)};
    forwarded_type ? last_type_.emplace(*forwarded_type) : resolve_ident(id, ident);

    auto& resolved{apply_explicit_modifiers(id, *last_type_.take())};
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

#define MAKE_MODIFIED_RESOLVER(NodeType, resolver)                                        \
    auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::NodeType& node) -> void { \
        PROFILE_FUNCTION();                                                               \
        resolver(id, node);                                                               \
        auto& resolved{apply_explicit_modifiers(id, *last_type_.take())};                 \
        resolving_.set_sema_type(id, resolved);                                           \
        last_type_.emplace(resolved);                                                     \
    }

MAKE_MODIFIED_RESOLVER(ModuleAccessExpression, resolve_module_access)
MAKE_MODIFIED_RESOLVER(DotExpression, resolve_dot)
MAKE_MODIFIED_RESOLVER(CallExpression, resolve_call)

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::ExplicitFunctionType& fn) -> void {
    PROFILE_FUNCTION();
    auto param_types{ctx_.pool.get_many_unsafe(fn.parameter_types.size())};

    // Make a unique function type by imprinting every associated type
    types::Key fn_key{TypeKind::FUNCTION, types::mut::CONSTANT};
    for (usize i{0}; const auto& param : fn.parameter_types) {
        TRY_RESOLVE(param);
        auto& param_type{*last_type_.take()};
        fn_key.imprint(param_type);
        param_types[i++] = &param_type;
    }

    TRY_RESOLVE(fn.explicit_return_type);
    auto& return_type{*last_type_.take()};
    fn_key.imprint(return_type);

    auto& resolved_fn{*ctx_.pool[fn_key]};
    resolved_fn.resolve_if<types::Function>(false, param_types, return_type);

    auto& final_type{apply_explicit_modifiers(id, resolved_fn)};
    resolving_.set_sema_type(id, final_type);
    last_type_.emplace(final_type);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, ast::ExplicitTypeID nested) -> void {
    PROFILE_FUNCTION();
    resolve(nested);
    auto& resolved{apply_explicit_modifiers(id, *last_type_.take())};
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::ExplicitArrayType& array) -> void {
    PROFILE_FUNCTION();
    TRY_RESOLVE(array.inner_explicit_type);
    auto& item_type{*last_type_.take()};

    const auto null_terminated{array.null_terminated};
    if (array.dimension) {
        // This is not an error for slices since slices are just pointers with length
        if (!item_type.is_resolved()) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                incomplete_array_item(resolving_.ast.location_of(array.inner_explicit_type))));
        }

        TRY_RESOLVE(*array.dimension);
        last_type_.emplace(ctx_.pool[{TypeKind::TYPE, types::mut::CONSTANT, &array}]);
        last_type_->resolve_if<types::DeferredArray>(array, item_type);
    } else {
        last_type_.emplace(ctx_.get_slice(types::mut::CONSTANT, null_terminated, item_type));
    }

    auto& final_type{apply_explicit_modifiers(id, *last_type_.take())};
    resolving_.set_sema_type(id, final_type);
    last_type_.emplace(final_type);
}

} // namespace ghoti::sema
