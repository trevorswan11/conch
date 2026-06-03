#pragma once

#include <concepts>
#include <span>
#include <string_view>

#include <ankerl/unordered_dense.h>

#include "ast/expression.hh"
#include "module/module.hh"

#include "arena.hh"
#include "enum.hh"
#include "fixed/vector.hh"
#include "hash.hh"
#include "memory.hh"
#include "option.hh"
#include "type_traits.hh"
#include "types.hh"
#include "utility.hh"
#include "variant.hh"

namespace ghoti::sema {

enum class TypeKind : u8 {
    POISON,
    I32,
    I64,
    ISIZE,
    U32,
    U64,
    USIZE,
    U8,
    BOOL,
    F32,
    F64,
    VOID,
    UNDEFINED,
    TYPE,
    SLICE,
    ARRAY,
    POINTER,
    REFERENCE,
    ENUM,
    STRUCT,
    UNION,
    FUNCTION,
    LABEL,
    BLOCK,
    MATCH_ARM,
    MODULE,

    AUTO,
    OPAQUE,
    NORETURN,
};

[[nodiscard]] auto type_kind_display_name(TypeKind kind) noexcept -> std::string_view;

class Type;

namespace types {

struct Poison {};

using BuiltinType = Unit;

struct Slice {
    Type& underlying;
    bool  null_terminated;
};

struct Array {
    Type&     underlying;
    opt::Size len; // This is empty for array types only to be set for constant eval
    bool      null_terminated;
};

struct Pointer {
    Type& underlying;
};

struct Reference {
    Type& underlying;
};

struct Enum {
    usize                         enumeration_count;
    bool                          non_exhaustive;
    Type&                         underlying;
    std::span<mem::NonNull<Type>> members;
};

struct Union {
    std::span<mem::NonNull<Type>> fields;
    std::span<mem::NonNull<Type>> members;
};

struct Struct {
    std::span<mem::NonNull<Type>> fields;
    std::span<mem::NonNull<Type>> members;
};

struct Function {
    bool                          has_self;
    std::span<mem::NonNull<Type>> params;
    Type&                         return_type;
};

struct Module {
    mod::Module& imported;
};

constexpr usize MAX_BUILTIN_PARAMS{4};
using BuiltinParams = fixed::Vector<mem::NonNull<Type>, MAX_BUILTIN_PARAMS>;

struct BuiltinFunction {
    BuiltinParams params;
    Type&         return_type;
};

struct MetaType {
    Type& instance;
};

// Represents a node who's type cannot be known until constant evaluation
struct DeferredEval {
    const ast::CallExpression& call_node;
};

enum class MutabilityModifiers : u8 {
    CONSTANT = 1 << 0,
    VOLATILE = 1 << 1,
};

MAKE_ENUM_OPERATORS(MutabilityModifiers)

class Key {
  public:
    template <typename... Markers>
    constexpr Key(TypeKind kind, MutabilityModifiers mut, Markers&&... markers) noexcept
        : kind_{kind}, mut_{mut} {
        (..., markers_.combine(markers));
    }

    MAKE_GETTER(kind, TypeKind)
    MAKE_GETTER(mut, MutabilityModifiers)

    auto set_kind(TypeKind kind) noexcept -> void { kind_ = kind; }
    auto set_mut(MutabilityModifiers mut) noexcept -> void { mut_ = mut; }

    // This is a high quality hash for the purposes of `unordered_dense`
    [[nodiscard]] constexpr auto hash() const noexcept -> u64 {
        hash::Hasher h{std::to_underlying(kind_)};
        h.combine(mut_);
        h.combine(markers_);
        return h.finalize();
    }

    // Emplace a hashable marker into the accumulated markers.
    //
    // WARNING: Only imprint onto keys when you are creating a new type. Making a modification to
    // mutability only (before rehashing) should never require a fresh imprint of the previous type!
    // Failing to follow this will completely brick the type checking mechanism.
    template <typename Marker> constexpr auto imprint(const Marker& marker) noexcept -> void {
        markers_.combine(marker);
    }

    constexpr auto clear_markers() noexcept -> void { markers_ = {}; }

    [[nodiscard]] constexpr auto operator==(const Key&) const noexcept -> bool = default;

  private:
    Key() noexcept = default;

  private:
    TypeKind            kind_;
    MutabilityModifiers mut_;
    hash::Hasher        markers_;

    friend class sema::Type;
};

namespace mut {

using types::MutabilityModifiers;

constexpr auto MUTABLE           = static_cast<types::MutabilityModifiers>(0);
constexpr auto CONSTANT          = MutabilityModifiers::CONSTANT;
constexpr auto VOLATILE          = MutabilityModifiers::VOLATILE;
constexpr auto CONSTANT_VOLATILE = CONSTANT | VOLATILE;

} // namespace mut

} // namespace types

} // namespace ghoti::sema

template <> struct ankerl::unordered_dense::hash<ghoti::sema::types::Key> {
    using is_avalanching = void;
    using Key            = ghoti::sema::types::Key;
    [[nodiscard]] auto operator()(const Key& key) const noexcept { return key.hash(); }
};

namespace ghoti::sema {

// A semantic type that is entirely owned by an arena of types
class Type {
  public:
    using Resolved = std::variant<types::Poison,
                                  types::BuiltinType,
                                  types::Slice,
                                  types::Array,
                                  types::Pointer,
                                  types::Reference,
                                  types::Enum,
                                  types::Union,
                                  types::Struct,
                                  types::Module,
                                  types::Function,
                                  types::BuiltinFunction,
                                  types::MetaType,
                                  types::DeferredEval>;

  public:
    ~Type() = default;

    Type(const Type&)                    = delete;
    auto operator=(const Type&) -> Type& = delete;
    Type(Type&&) noexcept                = delete;
    auto operator=(Type&&) -> Type&      = delete;

    MAKE_GETTER(key, const types::Key&)
    MAKE_OPTIONAL_UNPACKER(resolved, const Resolved&, resolved_, *)
    [[nodiscard]] auto get_resolved_opt() const noexcept { return resolved_; }
    [[nodiscard]] auto get_kind() const noexcept -> TypeKind { return key_.get_kind(); }

    // Unpacks T from the resolved type assuming the type has been resolved to T
    template <typename T> [[nodiscard]] auto as(this auto&& self) -> auto& {
        return std::get<T>(self.resolved_.value());
    }

    // Tries to unpack T, returning an empty option instead of throwing an exception
    template <typename T, typename Self>
    [[nodiscard]] auto as_opt(this Self&& self) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, T>&> {
        if (!self.resolved_ || !std::holds_alternative<T>(*self.resolved_)) { return opt::none; }
        return std::get<T>(*self.resolved_);
    }

    // Intended for use on pass 1 only
    constexpr auto set_symbol_table_idx(usize idx) noexcept -> void {
        symbol_table_idx_.emplace(idx);
    }

    MAKE_OPTIONAL_UNPACKER(symbol_table_idx, usize, symbol_table_idx_, *)
    [[nodiscard]] auto get_symbol_table_idx_opt() const noexcept { return symbol_table_idx_; }

    template <typename Resolvee, typename... Args> auto resolve(Args&&... args) noexcept -> void {
        resolved_.emplace(Resolvee{std::forward<Args>(args)...});
    }

    // Resolves only if not already resolved, returning true if modified
    template <typename Resolvee, typename... Args>
    auto resolve_if(Args&&... args) noexcept -> bool {
        if (has_resolved()) { return false; }
        resolved_.emplace(Resolvee{std::forward<Args>(args)...});
        return true;
    }

    // Returns the memory address of the Type for a Key's hash
    [[nodiscard]] constexpr auto hash() const noexcept -> u64 {
        return reinterpret_cast<u64>(this);
    }

    [[nodiscard]] constexpr auto is_poison() const noexcept -> bool {
        return key_.get_kind() == TypeKind::POISON;
    }

    [[nodiscard]] constexpr auto is_constant() const noexcept -> bool {
        return static_cast<bool>(key_.get_mut() & types::mut::CONSTANT);
    }

    [[nodiscard]] constexpr auto is_volatile() const noexcept -> bool {
        return static_cast<bool>(key_.get_mut() & types::mut::VOLATILE);
    }

    [[nodiscard]] constexpr auto operator==(const Type& other) const noexcept -> bool {
        return this == &other;
    }

  private:
    // This should only be used when allocating an immediately-to-be-filled span
    Type() noexcept = default;
    explicit Type(types::Key key) noexcept : key_{std::move(key)} {}

  private:
    types::Key            key_;
    opt::Size             symbol_table_idx_;
    opt::Option<Resolved> resolved_;

    // Initialization is restricted to the pool's arena exclusively
    friend class mem::Arena;
};

static_assert(traits::TriviallyDestructible<Type>);

// All associated type lifetimes are tied to the pool
class TypePool {
  public:
    TypePool() noexcept = default;
    ~TypePool()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TypePool)

    // Gets a type by its key or emplace's it into the internal cache
    [[nodiscard]] auto operator[](const types::Key& key) -> Type& { return get_or_emplace(key); }
    [[nodiscard]] auto get_opt(const types::Key& key) noexcept -> opt::Option<Type&>;

    // Allocate a quasi-contiguous span of types with the provided keys
    template <std::same_as<types::Key>... Keys>
    [[nodiscard]] auto get_many(Keys&&... keys) noexcept -> std::span<mem::NonNull<Type>> {
        auto  types = arena_.make_span<mem::NonNull<Type>>(sizeof...(Keys));
        usize i     = 0;
        (..., [&] { types[i++] = get_or_emplace(keys); }());
        return types;
    }

    // Allocates the requested number of types but does not initialize any data
    //
    // Violates an invariant of the NonNull class where the pointer is actually null
    [[nodiscard]] auto get_many_unsafe(usize count) noexcept -> std::span<mem::NonNull<Type>>;

    // Allocate a quasi-contiguous span of types with the same types
    [[nodiscard]] auto get_many(usize count, Type& common_type) noexcept
        -> std::span<mem::NonNull<Type>>;

    // Allocate a quasi-contiguous span of types with the same key types
    [[nodiscard]] auto get_many(usize count, types::Key common_key) noexcept
        -> std::span<mem::NonNull<Type>> {
        return get_many(count, get_or_emplace(common_key));
    }

    [[nodiscard]] auto strip_const(const Type& type) -> Type&;
    [[nodiscard]] auto strip_volatile(const Type& type) -> Type&;

  private:
    auto get_or_emplace(const types::Key& key) -> Type&;

  private:
    mem::Arena                                      arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace ghoti::sema

template <> struct ankerl::unordered_dense::hash<ghoti::sema::Type> {
    using is_avalanching = void;
    using Type           = ghoti::sema::Type;
    [[nodiscard]] auto operator()(const Type& type) const noexcept { return type.hash(); }
};
