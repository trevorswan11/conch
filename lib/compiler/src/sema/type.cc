#include "sema/type.hh"

#include <string_view>

#include <gsl/pointers>
#include <gsl/span>

#include <fixed/enum_map.hh>
#include <types.hh>

namespace ghoti::sema {

namespace {

using TypeMapping = std::pair<TypeKind, std::string_view>;

constexpr auto TYPE_KIND_NAMES{
    fixed::EnumMap<TypeKind, std::string_view>::from({},
                                                     TypeMapping{TypeKind::POISON, "poison"},
                                                     TypeMapping{TypeKind::I32, "i32"},
                                                     TypeMapping{TypeKind::I64, "i64"},
                                                     TypeMapping{TypeKind::ISIZE, "isize"},
                                                     TypeMapping{TypeKind::U32, "u32"},
                                                     TypeMapping{TypeKind::U64, "u64"},
                                                     TypeMapping{TypeKind::USIZE, "usize"},
                                                     TypeMapping{TypeKind::U8, "u8"},
                                                     TypeMapping{TypeKind::BOOL, "bool"},
                                                     TypeMapping{TypeKind::F32, "f32"},
                                                     TypeMapping{TypeKind::F64, "f64"},
                                                     TypeMapping{TypeKind::VOID, "void"},
                                                     TypeMapping{TypeKind::UNDEFINED, "undefined"},
                                                     TypeMapping{TypeKind::TYPE, "type"},
                                                     TypeMapping{TypeKind::SLICE, "slice"},
                                                     TypeMapping{TypeKind::ARRAY, "array"},
                                                     TypeMapping{TypeKind::POINTER, "pointer"},
                                                     TypeMapping{TypeKind::REFERENCE, "reference"},
                                                     TypeMapping{TypeKind::ENUM, "enum"},
                                                     TypeMapping{TypeKind::STRUCT, "struct"},
                                                     TypeMapping{TypeKind::UNION, "union"},
                                                     TypeMapping{TypeKind::FUNCTION, "function"},
                                                     TypeMapping{TypeKind::LABEL, "label"},
                                                     TypeMapping{TypeKind::BLOCK, "block"},
                                                     TypeMapping{TypeKind::MATCH_ARM, "match arm"},
                                                     TypeMapping{TypeKind::MODULE, "module"},
                                                     TypeMapping{TypeKind::AUTO, "auto"},
                                                     TypeMapping{TypeKind::OPAQUE, "opaque"},
                                                     TypeMapping{TypeKind::NORETURN, "noreturn"})};

} // namespace

auto type_kind_display_name(TypeKind kind) noexcept -> std::string_view {
    return TYPE_KIND_NAMES[kind];
}

auto TypePool::get_or_emplace(const types::Key& key) -> gsl::not_null<Type*> {
    if (auto it{cache_.find(key)}; it != cache_.end()) { return it->second; }
    auto* type = arena_.make<Type>(key).get();
    cache_.emplace(key, type);
    return type;
}

auto TypePool::get_many_unsafe(usize count) noexcept -> gsl::span<Type*> {
    return arena_.make_span<Type*>(count);
}

auto TypePool::get_many(usize count, Type& common_type) noexcept -> gsl::span<Type*> {
    auto types{get_many_unsafe(count)};
    for (usize i{0}; i < count; ++i) { types[i] = &common_type; }
    return types;
}

namespace {

auto strip_modifiers(TypePool& pool, const Type& old_type, types::MutabilityModifiers mut)
    -> gsl::not_null<Type*> {
    auto key{old_type.get_key()};
    key.set_mut(key.get_mut() & ~mut);

    // Resolve here since the type information doesn't contain modifier information
    auto new_type{pool[key]};
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
