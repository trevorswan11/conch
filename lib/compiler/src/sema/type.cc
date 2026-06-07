#include "sema/type.hh"

#include <string_view>

#include <gsl/pointers>
#include <gsl/span>

#include <fixed/enum_map.hh>
#include <types.hh>

namespace ghoti::sema {

namespace {

constexpr auto TYPE_KIND_NAMES = [] {
    fixed::EnumMap<TypeKind, std::string_view> names;

    names[TypeKind::POISON]    = "poison";
    names[TypeKind::I32]       = "i32";
    names[TypeKind::I64]       = "i64";
    names[TypeKind::ISIZE]     = "isize";
    names[TypeKind::U32]       = "u32";
    names[TypeKind::U64]       = "u64";
    names[TypeKind::USIZE]     = "usize";
    names[TypeKind::U8]        = "u8";
    names[TypeKind::BOOL]      = "bool";
    names[TypeKind::F32]       = "f32";
    names[TypeKind::F64]       = "f64";
    names[TypeKind::VOID]      = "void";
    names[TypeKind::UNDEFINED] = "undefined";
    names[TypeKind::TYPE]      = "type";
    names[TypeKind::SLICE]     = "slice";
    names[TypeKind::ARRAY]     = "array";
    names[TypeKind::POINTER]   = "pointer";
    names[TypeKind::REFERENCE] = "reference";
    names[TypeKind::ENUM]      = "enum";
    names[TypeKind::STRUCT]    = "struct";
    names[TypeKind::UNION]     = "union";
    names[TypeKind::FUNCTION]  = "function";
    names[TypeKind::LABEL]     = "label";
    names[TypeKind::BLOCK]     = "block";
    names[TypeKind::MATCH_ARM] = "match arm";
    names[TypeKind::MODULE]    = "module";
    names[TypeKind::AUTO]      = "auto";
    names[TypeKind::OPAQUE]    = "opaque";
    names[TypeKind::NORETURN]  = "noreturn";

    return names;
}();

} // namespace

auto type_kind_display_name(TypeKind kind) noexcept -> std::string_view {
    return TYPE_KIND_NAMES[kind];
}

auto TypePool::get_or_emplace(const types::Key& key) -> gsl::not_null<Type*> {
    if (auto it = cache_.find(key); it != cache_.end()) { return it->second; }
    auto* type = arena_.make<Type>(key).get();
    cache_.emplace(key, type);
    return type;
}

auto TypePool::get_many_unsafe(usize count) noexcept -> gsl::span<Type*> {
    return arena_.make_span<Type*>(count);
}

auto TypePool::get_many(usize count, Type& common_type) noexcept -> gsl::span<Type*> {
    auto types = get_many_unsafe(count);
    for (usize i = 0; i < count; ++i) { types[i] = &common_type; }
    return types;
}

namespace {

auto strip_modifiers(TypePool& pool, const Type& old_type, types::MutabilityModifiers mut)
    -> gsl::not_null<Type*> {
    auto key = old_type.get_key();
    key.set_mut(key.get_mut() & ~mut);

    // Resolve here since the type information doesn't contain modifier information
    auto new_type = pool[key];
    new_type->resolve_if<Type::Data>(old_type.get_data());
    return new_type;
}

} // namespace

auto TypePool::strip_const(const Type& type) -> gsl::not_null<Type*> {
    if (!type.is_constant()) { return const_cast<Type*>(&type); }
    return strip_modifiers(*this, type, types::mut::CONSTANT);
}

auto TypePool::strip_volatile(const Type& type) -> gsl::not_null<Type*> {
    if (!type.is_volatile()) { return const_cast<Type*>(&type); }
    return strip_modifiers(*this, type, types::mut::VOLATILE);
}

} // namespace ghoti::sema
