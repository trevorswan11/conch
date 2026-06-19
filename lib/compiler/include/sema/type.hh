#pragma once

#include <concepts>
#include <string_view>

#include <ankerl/unordered_dense.h>
#include <gsl/pointers>
#include <gsl/span>
#include <stdx/arena.hh>
#include <stdx/assert.hh>
#include <stdx/enum.hh>
#include <stdx/fixed/vector.hh>
#include <stdx/hash.hh>
#include <stdx/option.hh>
#include <stdx/type_traits.hh>
#include <stdx/types.hh>
#include <stdx/utility.hh>
#include <stdx/variant.hh>

#include "ast/expression.hh"
#include "ast/type.hh"
#include "module/module.hh"

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

struct Unresolved {};
struct Poison {};

using BuiltinType = stdx::monostate;

struct Slice {
    Type& underlying;
    bool  null_terminated;
};

struct Array {
    Type& underlying;
    usize len;
    bool  null_terminated;
};

struct Pointer {
    Type& underlying;
};

struct Reference {
    Type& underlying;
};

struct Enum {
    gsl::span<const ast::EnumExpression::Enumeration> ast_enumerations;
    bool                                              non_exhaustive;
    Type&                                             underlying;
    gsl::span<Type*>                                  members;
    const mod::Module&                                enclosing;

    [[nodiscard]] auto type_at(usize idx, Type& object_type) const noexcept -> Type& {
        // The index location entirely depends on the number of variants which always come first
        const auto enumeration_count{ast_enumerations.size()};
        ASSERT(idx < enumeration_count + members.size(), "Index exceeds enum's types");
        if (idx < enumeration_count) { return object_type; }
        return *members[idx - enumeration_count];
    }
};

struct Union {
    gsl::span<Type*>                             fields;
    gsl::span<const ast::UnionExpression::Field> ast_fields;
    gsl::span<Type*>                             members;
    const mod::Module&                           enclosing;

    // The index location entirely depends on the number of fields which always come first
    [[nodiscard]] auto type_at(usize idx) const noexcept -> Type& {
        ASSERT(idx < fields.size() + members.size(), "Index exceeds union's types");
        if (idx < fields.size()) { return *fields[idx]; }
        return *members[idx - fields.size()];
    }
};

struct Struct {
    gsl::span<Type*>                              fields;
    gsl::span<const ast::StructExpression::Field> ast_fields;
    gsl::span<Type*>                              members;
    const mod::Module&                            enclosing;

    // The index location entirely depends on the number of fields which always come first
    [[nodiscard]] auto type_at(usize idx) const noexcept -> Type& {
        ASSERT(idx < fields.size() + members.size(), "Index exceeds struct's types");
        if (idx < fields.size()) { return *fields[idx]; }
        return *members[idx - fields.size()];
    }
};

struct Function {
    bool             has_self;
    gsl::span<Type*> params;
    Type&            return_type;
};

struct Module {
    mod::Module& imported;
};

constexpr usize MAX_BUILTIN_PARAMS{4};
using BuiltinParams = stdx::fixed::vector<gsl::not_null<Type*>, MAX_BUILTIN_PARAMS>;

struct BuiltinFunction {
    BuiltinParams params;
    Type&         return_type;
};

struct MetaType {
    Type& instance;
};

struct DeferredCall {
    const ast::CallExpression& call;
};

struct DeferredArray {
    const ast::ExplicitArrayType& array;
    Type&                         underlying;
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
        stdx::hash::hasher h{std::to_underlying(kind_)};
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
    stdx::hash::hasher  markers_;

    friend class sema::Type;
};

namespace mut {

using types::MutabilityModifiers;

constexpr auto MUTABLE{static_cast<types::MutabilityModifiers>(0)};
constexpr auto CONSTANT{MutabilityModifiers::CONSTANT};
constexpr auto VOLATILE{MutabilityModifiers::VOLATILE};
constexpr auto CONSTANT_VOLATILE{CONSTANT | VOLATILE};

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
    using Data = stdx::variant<types::Unresolved,
                               types::Poison,
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
                               types::DeferredCall,
                               types::DeferredArray>;

  public:
    ~Type() = default;

    Type(const Type&)                    = delete;
    auto operator=(const Type&) -> Type& = delete;
    Type(Type&&) noexcept                = delete;
    auto operator=(Type&&) -> Type&      = delete;

    MAKE_GETTER(key, const types::Key&)
    MAKE_DEDUCING_GETTER(data)

    [[nodiscard]] auto get_kind() const noexcept -> TypeKind { return key_.get_kind(); }

    // Intended for use on pass 1 only
    constexpr auto set_symbol_table_idx(usize idx) noexcept -> void {
        symbol_table_idx_.emplace(idx);
    }

    [[nodiscard]] auto has_symbol_table_idx() const noexcept -> bool {
        return symbol_table_idx_.has_value();
    }

    [[nodiscard]] auto get_symbol_table_idx() const noexcept { return *symbol_table_idx_; }
    [[nodiscard]] auto get_symbol_table_idx_opt() const noexcept { return symbol_table_idx_; }

    [[nodiscard]] auto is_resolved() const noexcept -> bool {
        return !data_.is<types::Unresolved>();
    }

    auto unresolve() noexcept -> void { data_.emplace<types::Unresolved>(); }
    template <typename Resolvee, typename... Args> auto resolve(Args&&... args) noexcept -> void {
        data_.emplace<Resolvee>(std::forward<Args>(args)...);
    }

    // Resolves only if not already resolved, returning true if modified
    template <typename Resolvee, typename... Args>
    auto resolve_if(Args&&... args) noexcept -> bool {
        if (is_resolved()) { return false; }
        data_.emplace<Resolvee>(std::forward<Args>(args)...);
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
    explicit Type(types::Key key) noexcept : key_{key} {}

  private:
    types::Key     key_;
    stdx::opt_size symbol_table_idx_;
    Data           data_;

    // Initialization is restricted to the pool's arena exclusively
    friend class stdx::arena;
};

static_assert(stdx::TriviallyDestructible<Type>);

// All associated type lifetimes are tied to the pool
class TypePool {
  public:
    TypePool() noexcept = default;
    ~TypePool()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TypePool)

    // Gets a type by its key or emplace's it into the internal cache
    [[nodiscard]] auto operator[](const types::Key& key) -> gsl::not_null<Type*> {
        return get_or_emplace(key);
    }

    // Allocate a quasi-contiguous span of types with the provided keys
    template <std::same_as<types::Key>... Keys>
    [[nodiscard]] auto get_many(Keys&&... keys) noexcept -> gsl::span<Type*> {
        auto  types{arena_.make_span<Type*>(sizeof...(Keys))};
        usize i{0};
        (..., (types[i++] = get_or_emplace(keys)));
        return types;
    }

    // Allocates the requested number of types but does not initialize any data
    [[nodiscard]] auto get_many_unsafe(usize count) noexcept -> gsl::span<Type*>;

    // Allocate a quasi-contiguous span of types with the same types
    [[nodiscard]] auto get_many(usize count, Type& common_type) noexcept -> gsl::span<Type*>;

    // Allocate a quasi-contiguous span of types with the same key types
    [[nodiscard]] auto get_many(usize count, types::Key common_key) noexcept -> gsl::span<Type*> {
        return get_many(count, *get_or_emplace(common_key));
    }

    [[nodiscard]] auto strip_const(const Type& type) -> gsl::not_null<Type*>;
    [[nodiscard]] auto strip_volatile(const Type& type) -> gsl::not_null<Type*>;

  private:
    auto get_or_emplace(const types::Key& key) -> gsl::not_null<Type*>;

  private:
    stdx::arena                                     arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace ghoti::sema

template <> struct ankerl::unordered_dense::hash<ghoti::sema::Type> {
    using is_avalanching = void;
    using Type           = ghoti::sema::Type;
    [[nodiscard]] auto operator()(const Type& type) const noexcept { return type.hash(); }
};
