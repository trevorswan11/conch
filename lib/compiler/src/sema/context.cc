#include "sema/context.hh"

#include <concepts>
#include <utility>

#include <gsl/pointers>

#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"
#include "syntax/keywords.hh"

#include <assert.hh>
#include <types.hh>

namespace ghoti::sema {

auto Context::get_poison() -> Type& {
    auto& poison = *pool[{TypeKind::POISON, types::mut::CONSTANT}];
    poison.resolve_if<types::Poison>();
    return poison;
}

auto Context::get_pointer(types::mut::MutabilityModifiers mutability, Type& underlying) -> Type& {
    auto& type = *pool[{TypeKind::POINTER, mutability, underlying}];
    type.resolve_if<types::Pointer>(underlying);
    return type;
}

auto Context::get_reference(types::mut::MutabilityModifiers mutability, Type& underlying) -> Type& {
    auto& type = *pool[{TypeKind::REFERENCE, mutability, underlying}];
    type.resolve_if<types::Reference>(underlying);
    return type;
}

auto Context::get_array(types::mut::MutabilityModifiers mutability,
                        bool                            null_terminated,
                        usize                           size,
                        Type&                           underlying) -> Type& {
    auto& type = *pool[{TypeKind::ARRAY, mutability, null_terminated, size, underlying}];
    type.resolve_if<types::Array>(underlying, size, null_terminated);
    return type;
}

auto Context::get_slice(types::mut::MutabilityModifiers mutability,
                        bool                            null_terminated,
                        Type&                           underlying) -> Type& {
    auto& type = *pool[{TypeKind::SLICE, mutability, null_terminated, underlying}];
    type.resolve_if<types::Slice>(underlying, null_terminated);
    return type;
}

namespace {

auto inject_types(SymbolTable& prelude, TypePool& pool) -> void {
    const auto inject_type = [&](const syntax::Keyword& keyword, TypeKind kind) -> void {
        auto& type = *pool[{kind, types::mut::CONSTANT}];
        ASSERT(!type.is_resolved(), "Builtin types should only be resolved once");
        type.resolve<types::BuiltinType>();

        prelude.insert_unchecked(keyword.name, symbols::Builtin{keyword, type});
        auto& symbol = prelude.get(keyword.name);
        symbol.set_kind(SymbolKind::TYPE);
        symbol.set_status(SymbolStatus::RESOLVED);
    };

    namespace kws = syntax::keywords;

    // Primitives
    inject_type(kws::I32, TypeKind::I32);
    inject_type(kws::I64, TypeKind::I64);
    inject_type(kws::ISIZE, TypeKind::ISIZE);
    inject_type(kws::U32, TypeKind::U32);
    inject_type(kws::U64, TypeKind::U64);
    inject_type(kws::USIZE, TypeKind::USIZE);
    inject_type(kws::F32, TypeKind::F32);
    inject_type(kws::F64, TypeKind::F64);
    inject_type(kws::U8, TypeKind::U8);
    inject_type(kws::BOOL, TypeKind::BOOL);
    inject_type(kws::VOID, TypeKind::VOID);

    // Special types
    inject_type(kws::TYPE, TypeKind::TYPE);
    inject_type(kws::AUTO, TypeKind::AUTO);
    inject_type(kws::OPAQUE, TypeKind::OPAQUE);
    inject_type(kws::UNDEFINED, TypeKind::UNDEFINED);
    inject_type(kws::NORETURN, TypeKind::NORETURN);
}

auto inject_functions(SymbolTable& prelude, TypePool& pool) -> void {
    const auto inject_function = [&](const syntax::Builtin& builtin,
                                     types::BuiltinParams&& param_types,
                                     Type&                  return_type) -> void {
        for (const auto& param_type : param_types) {
            ASSERT(param_type->is_resolved(), "Builtins must be fully resolved");
        }
        ASSERT(return_type.is_resolved(), "Builtins must be fully resolved");

        types::Key key{TypeKind::FUNCTION, types::mut::CONSTANT};
        key.imprint(builtin);
        auto& type = *pool[key];
        ASSERT(!type.is_resolved(), "Builtin functions should only be resolved once");
        type.resolve<types::BuiltinFunction>(std::move(param_types), return_type);

        prelude.insert_unchecked(builtin.name, symbols::Builtin{builtin, type});
        auto& symbol = prelude.get(builtin.name);
        symbol.set_kind(SymbolKind::CALLABLE);
        symbol.set_status(SymbolStatus::RESOLVED);
    };

    namespace bis     = syntax::builtins;
    const auto params = [&](std::same_as<Type&> auto&&... params) -> auto {
        return types::BuiltinParams{&params...};
    };

    // Common types
    auto& t_void     = *pool[{TypeKind::VOID, types::mut::CONSTANT}];
    auto& t_type     = *pool[{TypeKind::TYPE, types::mut::CONSTANT}];
    auto& t_usize    = *pool[{TypeKind::USIZE, types::mut::CONSTANT}];
    auto& t_auto     = *pool[{TypeKind::AUTO, types::mut::CONSTANT}];
    auto& t_noreturn = *pool[{TypeKind::NORETURN, types::mut::CONSTANT}];

    // C-string
    auto& t_u8    = *pool[{TypeKind::U8, types::mut::CONSTANT}];
    auto& t_c_str = *pool[{TypeKind::SLICE, types::mut::CONSTANT, true, t_u8}];
    t_c_str.resolve_if<types::Slice>(*pool[{TypeKind::U8, types::mut::CONSTANT}], true);

    inject_function(bis::ALIGN_CAST, params(t_type, t_auto), t_auto);
    inject_function(bis::PTR_CAST, params(t_type, t_auto), t_auto);
    inject_function(bis::BIT_CAST, params(t_type, t_auto), t_auto);
    inject_function(bis::CONST_CAST, params(t_auto), t_auto);
    inject_function(bis::VOLATILE_CAST, params(t_auto), t_auto);
    inject_function(bis::AS, params(t_type, t_auto), t_auto);

    inject_function(bis::INT_FROM_PTR, params(t_auto), t_usize);
    inject_function(bis::PTR_FROM_INT, params(t_type, t_usize), t_auto);
    inject_function(bis::PTR_FROM_ARRAY, params(t_auto), t_auto);
    inject_function(bis::SLICE_FROM_PTR, params(t_auto, t_usize), t_auto);

    inject_function(bis::ALIGN_OF, params(t_auto), t_usize);
    inject_function(bis::SIZE_OF, params(t_auto), t_usize);
    inject_function(bis::TYPE_OF, params(t_auto), t_type);
    inject_function(bis::THIS, params(), t_type);
    inject_function(bis::TAG_NAME, params(t_auto), t_c_str);

    inject_function(bis::MEMCPY, params(t_auto, t_auto), t_void);
    inject_function(bis::MEMSET, params(t_auto, t_auto), t_void);
    inject_function(bis::MEMMOVE, params(t_auto, t_auto), t_void);

    inject_function(bis::MUL_ADD, params(t_type, t_auto, t_auto, t_auto), t_auto);
    inject_function(bis::CLZ, params(t_auto), t_usize);
    inject_function(bis::CTZ, params(t_auto), t_usize);
    inject_function(bis::POP_COUNT, params(t_auto), t_usize);
    inject_function(bis::SQRT, params(t_auto), t_auto);
    inject_function(bis::SIN, params(t_auto), t_auto);
    inject_function(bis::COS, params(t_auto), t_auto);
    inject_function(bis::TAN, params(t_auto), t_auto);
    inject_function(bis::EXP, params(t_auto), t_auto);
    inject_function(bis::EXP2, params(t_auto), t_auto);
    inject_function(bis::LOG, params(t_auto), t_auto);
    inject_function(bis::LOG2, params(t_auto), t_auto);
    inject_function(bis::LOG10, params(t_auto), t_auto);
    inject_function(bis::ABS, params(t_auto), t_auto);
    inject_function(bis::FLOOR, params(t_auto), t_auto);
    inject_function(bis::CEIL, params(t_auto), t_auto);

    inject_function(bis::PANIC, params(t_c_str), t_noreturn);
}

} // namespace

auto Context::inject_prelude() -> void {
    if (prelude_index) { return; }
    prelude_index.emplace(registry.create());

    auto& prelude = registry.get(*prelude_index);
    inject_types(prelude, pool);
    inject_functions(prelude, pool);
}

auto Context::get_builtin_resolved_type(TypeKind kind) -> Type& {
    auto& type = *pool[{kind, types::mut::CONSTANT}];
    ASSERT(type.is_resolved(), "Builtin type was not already resolved");
    return type;
}

} // namespace ghoti::sema
