#pragma once

#include <ostream>
#include <utility>

#include <gsl/pointers>
#include <stdx/option.hh>
#include <stdx/result.hh>
#include <stdx/types.hh>

#include "ast/traits.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

namespace ghoti::sema {

// A contextual wrapper around sematic steps
//
// Owns its own diagnostic list
struct Context {
    mod::ModuleManager&  modules;
    SymbolTableRegistry& registry;
    TypePool&            pool;
    Diagnostics          diagnostics;
    std::ostream&        error_stream;
    stdx::opt_size        prelude_index;

    Context(mod::ModuleManager&  modules,
            SymbolTableRegistry& registry,
            TypePool&            pool,
            Diagnostics          diagnostics,
            std::ostream&        error_stream) noexcept
        : modules{modules}, registry{registry}, pool{pool}, diagnostics{std::move(diagnostics)},
          error_stream{error_stream} {}
    ~Context() = default;

    // Creates a copy with identical data but a new diagnostic list
    Context(const Context& other)
        : modules{other.modules}, registry{other.registry}, pool{other.pool},
          diagnostics{other.diagnostics.create_new()}, error_stream{other.error_stream},
          prelude_index{other.prelude_index} {}

    auto operator=(const Context& other) -> Context& = delete;
    Context(Context&&) noexcept                      = default;
    auto operator=(Context&&) -> Context&            = delete;

    // Returns false if the passed result was an error type, which is forwarded to the diagnostics
    template <typename T = void> auto try_result(stdx::result<T, Diagnostic>&& result) -> bool {
        if (!result) {
            diagnostics.emplace_back(result.error());
            return false;
        }
        return true;
    }

    // Gets the already-resolved poison type from the pool
    [[nodiscard]] auto get_poison() -> Type&;

    // Calls resolve_if on the resulting type
    [[nodiscard]] auto get_pointer(types::mut::MutabilityModifiers mutability, Type& underlying)
        -> Type&;

    // Calls resolve_if on the resulting type
    [[nodiscard]] auto get_reference(types::mut::MutabilityModifiers mutability, Type& underlying)
        -> Type&;

    // Calls resolve_if on the resulting type
    [[nodiscard]] auto get_array(types::mut::MutabilityModifiers mutability,
                                 bool                            null_terminated,
                                 usize                           size,
                                 Type&                           underlying) -> Type&;

    // Calls resolve_if on the resulting type
    [[nodiscard]] auto get_slice(types::mut::MutabilityModifiers mutability,
                                 bool                            null_terminated,
                                 Type&                           underlying) -> Type&;

    // Poisons the symbol and constructs an associated diagnostic to insert into the list
    template <typename... Args> auto poison_symbol(Symbol& symbol, Args&&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            diagnostics.emplace_back(std::forward<Args>(args)...);
        }
        symbol.set_kind(SymbolKind::POISONED);
        symbol.set_status(SymbolStatus::RESOLVED);
    }

    // Poisons the node and constructs an associated diagnostic to insert into the list
    //
    // Returns the poison type for optional non-lookup usage
    template <traits::IndexableID ID, typename... Args>
    auto poison_node(mod::Module& module, ID id, Args&&... args) -> Type& {
        if constexpr (sizeof...(args) != 0) {
            diagnostics.emplace_back(std::forward<Args>(args)...);
        }

        auto& poison{get_poison()};
        module.set_sema_type(id, poison);
        return poison;
    }

    // Creates and injects the builtin/primitive prelude and sets the internal prelude index
    auto inject_prelude() -> void;

    // Convenience function for retrieving constant builtin types
    [[nodiscard]] auto get_builtin_resolved_type(TypeKind kind) -> Type&;
};

} // namespace ghoti::sema
