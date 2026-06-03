#pragma once

#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "iterator.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "type_traits.hh"
#include "types.hh"
#include "utility.hh"
#include "variant.hh"

namespace ghoti::sema {

class Type;
class Symbol;

enum class SymbolKind : u8 {
    TYPE,
    VALUE,
    CALLABLE,
    MODULE,
    LABEL,
    POISONED,
};

enum class SymbolStatus : u8 {
    UNRESOLVED,
    RESOLVING,
    RESOLVED,
};

namespace symbols {

// A non-AST-based symbol for builtin types and functions
//
// Holds its own semantic type since it cannot be stored in an AST side table
class Builtin {
  public:
    explicit Builtin(const syntax::TypedIdentifier& tok, Type& type) noexcept
        : token_{tok}, type_{type} {}

    MAKE_GETTER(token, const syntax::Token&)
    MAKE_GETTER(type, Type&)

  private:
    syntax::Token token_;
    Type&         type_;
};

using Node = ast::Handle<ast::NodeKind::DECL_STATEMENT,
                         ast::NodeKind::USING_STATEMENT,
                         ast::NodeKind::IMPORT_STATEMENT>;

class Label {
  public:
    using Handle = ast::Handle<ast::NodeKind::LABEL_EXPRESSION>;

  public:
    explicit Label(Handle label) noexcept : definition_{label} {}

    MAKE_GETTER(definition, Handle)
    MAKE_GETTER(yield_types, std::span<const mem::NonNull<Type>>)

    [[nodiscard]] auto has_yield_types() const noexcept -> bool { return !yield_types_.empty(); }
    auto               add_yield_type(Type& type) -> void { yield_types_.emplace_back(type); }

    // Gets the Label data from the symbol, asserting the underlying data is a label
    [[nodiscard]] static auto from(Symbol& symbol) -> Label&;

  private:
    Handle                          definition_;
    std::vector<mem::NonNull<Type>> yield_types_;
};

using MatchCapture   = ast::IdentifierHandle;
using StructField    = ast::StructExpression::Field;
using UnionField     = ast::UnionExpression::Field;
using Enumeration    = ast::EnumExpression::Enumeration;
using SelfParameter  = ast::SelfParameter;
using Parameter      = ast::FunctionExpression::Parameter;
using ForLoopCapture = ast::ForLoopExpression::Capture;

} // namespace symbols

class Symbol {
  public:
    using Data = std::variant<symbols::Builtin,
                              symbols::Node,
                              symbols::Label,
                              symbols::MatchCapture,
                              symbols::StructField,
                              symbols::UnionField,
                              symbols::Enumeration,
                              symbols::SelfParameter,
                              symbols::Parameter,
                              symbols::ForLoopCapture>;

  public:
    Symbol(std::string_view name, const Data& data) noexcept : name_{name}, data_{data} {}
    ~Symbol() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Symbol)

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(data, const Symbol::Data&)

    // Unpacks T from the resolved type assuming the type has been resolved to T
    template <typename T> [[nodiscard]] auto as(this auto&& self) -> auto& {
        return std::get<T>(self.data_);
    }

    // Tries to unpack T, returning an empty option instead of throwing an exception
    template <typename T, typename Self>
    [[nodiscard]] auto as_opt(this Self&& self) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, T>&> {
        if (!std::holds_alternative<T>(self.data_)) { return opt::none; }
        return std::get<T>(self.data_);
    }

    MAKE_VARIANT_MATCHER(data_)

    [[nodiscard]] auto get_symbol_location(const mod::Module& module) const noexcept
        -> SourceLocation;

    // Can only be true for decls, imports, and type aliases
    [[nodiscard]] auto is_public(const mod::Module& module) const noexcept -> bool;

    [[nodiscard]] auto get_status() const noexcept -> SymbolStatus { return status_; }
    auto               set_status(SymbolStatus status) noexcept -> void { status_ = status; }

    [[nodiscard]] auto has_kind() const noexcept -> bool { return kind_.has_value(); }
    [[nodiscard]] auto get_kind_opt() const noexcept -> opt::Option<SymbolKind> { return kind_; }
    [[nodiscard]] auto get_kind() const noexcept -> SymbolKind { return *get_kind_opt(); }
    auto               set_kind(SymbolKind kind) noexcept -> void { kind_ = kind; }

  private:
    std::string_view        name_;
    Data                    data_;
    SymbolStatus            status_{SymbolStatus::UNRESOLVED};
    opt::Option<SymbolKind> kind_;
};

class SymbolTable {
  public:
    // The managed value type in mapped to by inserted keys
    struct ValueProxy {
        Symbol symbol;
        usize  idx;
    };

    // Used for returning pseudo reference types from get operations, avoided in iterators
    template <typename Self> struct ReferenceProxy {
        traits::const_dispatch_t<Self, Symbol>& symbol;
        usize                                   index;
    };

    using Table = ankerl::unordered_dense::map<std::string_view, ValueProxy>;
    MAKE_UNALIASED_ITERATOR(Table, symbols_)
    using KV = Table::iterator::value_type;

  public:
    SymbolTable() noexcept = default;
    ~SymbolTable()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTable)

    // Constructs the symbolic node in place with the provided args
    template <typename T, typename... Args>
    auto insert(std::string_view name, const mod::Module& module, Args&&... args)
        -> Result<void, Diagnostic> {
        return insert(name, module, Symbol::Data{T{std::forward<Args>(args)...}});
    }

    // Checks that the module was inserted without collision
    auto insert(std::string_view name, const mod::Module& module, const Symbol::Data& data)
        -> Result<void, Diagnostic>;

    // For use of prelude injection only
    auto insert_unchecked(std::string_view name, const Symbol::Data& data) -> void;

    auto reserve(usize cap) -> void { symbols_.reserve(cap); }

    [[nodiscard]] auto has(std::string_view name) const noexcept -> bool {
        return symbols_.contains(name);
    }

    // Differs from `get_proxy_opt` by asserting that the name is present.
    template <typename Self>
    [[nodiscard]] auto get_proxy(this Self&& self, std::string_view name) noexcept {
        auto it = self.symbols_.find(name);
        ASSERT(it != self.symbols_.end(), "Illegal get on missing key");
        return ReferenceProxy<Self>{it->second.symbol, it->second.idx};
    }

    // Returns optional referential metadata of the symbol if present
    template <typename Self>
    [[nodiscard]] auto get_proxy_opt(this Self&& self, std::string_view name) noexcept
        -> opt::Option<ReferenceProxy<Self>> {
        auto it = self.symbols_.find(name);
        if (it == self.symbols_.end()) { return opt::none; }
        return ReferenceProxy<Self>{it->second.symbol, it->second.idx};
    }

    // Differs from `get_opt` by asserting that the name is present.
    [[nodiscard]] auto get(this auto&& self, std::string_view name) noexcept -> auto& {
        return self.get_proxy(name).symbol;
    }

    // Returns an optional containing a mutable or const reference to a symbol depending on context.
    template <typename Self>
    [[nodiscard]] auto get_opt(this Self&& self, std::string_view name) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, Symbol>&> {
        const auto proxy = self.get_proxy_opt(name);
        if (!proxy) { return opt::none; }
        return proxy->symbol;
    }

  private:
    Table symbols_;
};

class SymbolTableStack {
  public:
    // A basic push/pop RAII guard, see `Scope`
    class Guard {
      public:
        Guard(SymbolTableStack& s, usize idx) noexcept : stack_{s} { stack_.push(idx); }
        ~Guard() { stack_.pop(); }

      private:
        SymbolTableStack& stack_;
    };

    // An extension of `Guard` that also resets the old index upon destruction
    class Scope {
      public:
        Scope(SymbolTableStack& s, usize new_idx, usize& old_idx) noexcept
            : guard_{s, new_idx}, idx_ref_{old_idx}, old_idx_{old_idx} {
            old_idx = new_idx;
        }
        ~Scope() { idx_ref_ = old_idx_; }

      private:
        SymbolTableStack::Guard guard_;
        usize&                  idx_ref_;
        usize                   old_idx_;
    };

    MAKE_ITERATOR(Stack, std::vector<usize>, stack_)

  public:
    SymbolTableStack() noexcept = default;
    ~SymbolTableStack()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTableStack)

    auto push(usize idx) -> void { stack_.push_back(idx); }
    auto pop() noexcept -> void {
        if (!stack_.empty()) { stack_.pop_back(); }
    }

  private:
    Stack stack_;
};

class SymbolTableRegistry {
  public:
    MAKE_ITERATOR(Tables, std::vector<SymbolTable>, tables_)

  public:
    SymbolTableRegistry() noexcept = default;
    ~SymbolTableRegistry()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTableRegistry)

    [[nodiscard]] auto create() -> usize {
        tables_.emplace_back();
        return tables_.size() - 1;
    }

    // Constructs the symbolic node in place with the provided args
    template <typename T, typename... Args>
    [[nodiscard]] auto
    insert_into(usize table_idx, const mod::Module& module, std::string_view name, Args&&... args)
        -> Result<void, Diagnostic> {
        return insert_into(table_idx, module, name, Symbol::Data{T{std::forward<Args>(args)...}});
    }

    [[nodiscard]] auto insert_into(usize               table_idx,
                                   const mod::Module&  module,
                                   std::string_view    name,
                                   const Symbol::Data& data) -> Result<void, Diagnostic>;

    [[nodiscard]] auto get(this auto&& self, usize idx) -> auto& { return self.tables_.at(idx); }

    template <typename Self>
    [[nodiscard]] auto get_opt(this Self&& self, usize idx) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, SymbolTable>&> {
        if (idx >= self.tables_.size()) { return opt::none; }
        return self.tables_[idx];
    }

    [[nodiscard]] auto get_from(this auto&& self, usize idx, std::string_view name) -> auto& {
        return self.tables_.at(idx).get(name);
    }

    template <typename Self>
    [[nodiscard]] auto get_from_opt(this Self&& self, usize idx, std::string_view name) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, Symbol>&> {
        if (idx >= self.tables_.size()) { return opt::none; }
        return self.tables_[idx].get_opt(name);
    }

    // Looks up all levels of the stack for possible illegal shadowing of the name
    [[nodiscard]] auto is_shadowing(const SymbolTableStack& stack,
                                    const mod::Module&      module,
                                    std::string_view        name,
                                    const Symbol::Data& data) noexcept -> Result<void, Diagnostic>;

    // Looks up all levels of the stack for the queried name
    template <typename Self>
    [[nodiscard]] auto
    lookup(this Self&& self, const SymbolTableStack& stack, std::string_view name) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, Symbol>&> {
        for (const auto idx : std::views::reverse(stack)) {
            if (auto symbol = self.tables_[idx].get_opt(name)) { return symbol; }
        }
        return opt::none;
    }

  private:
    Tables tables_;
};

#undef OPTIONAL_RETURN_TYPE

} // namespace ghoti::sema
