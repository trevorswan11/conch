#include "sema/context.hh"

#include <utility>

#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"
#include "syntax/keywords.hh"

#include "assert.hh"

namespace ghoti::sema {

auto Context::get_poison() -> Type& {
    auto& poison = pool[{TypeKind::POISON, types::mut::CONSTANT}];
    poison.resolve_if<types::Poison>();
    return poison;
}

namespace {

auto inject_types(SymbolTable& prelude, TypePool& pool) -> void {
    const auto inject_type = [&](const syntax::Keyword& keyword, TypeKind kind) {
        auto& type = pool[{kind, types::mut::CONSTANT}];
        ASSERT(!type.has_resolved(), "Builtin types should only be resolved once");
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
    const auto inject_function =
        [&](const syntax::Builtin& builtin, types::BuiltinParams&& param_types, Type& return_type) {
            for (const auto& param_type : param_types) {
                ASSERT(param_type->has_resolved(), "Builtins must be fully resolved");
            }
            ASSERT(return_type.has_resolved(), "Builtins must be fully resolved");

            types::Key key{TypeKind::FUNCTION, types::mut::CONSTANT};
            key.imprint(builtin);
            auto& type = pool[key];
            ASSERT(!type.has_resolved(), "Builtin functions should only be resolved once");
            type.resolve<types::BuiltinFunction>(std::move(param_types), return_type);

            prelude.insert_unchecked(builtin.name, symbols::Builtin{builtin, type});
            auto& symbol = prelude.get(builtin.name);
            symbol.set_kind(SymbolKind::CALLABLE);
            symbol.set_status(SymbolStatus::RESOLVED);
        };

    namespace bis = syntax::builtins;
    using BP      = types::BuiltinParams;

    // Common types
    auto& t_void     = pool[{TypeKind::VOID, types::mut::CONSTANT}];
    auto& t_type     = pool[{TypeKind::TYPE, types::mut::CONSTANT}];
    auto& t_usize    = pool[{TypeKind::USIZE, types::mut::CONSTANT}];
    auto& t_auto     = pool[{TypeKind::AUTO, types::mut::CONSTANT}];
    auto& t_noreturn = pool[{TypeKind::NORETURN, types::mut::CONSTANT}];

    // C-string
    auto& t_u8    = pool[{TypeKind::U8, types::mut::CONSTANT}];
    auto& t_c_str = pool[{TypeKind::SLICE, types::mut::CONSTANT, true, t_u8}];
    t_c_str.resolve_if<types::Slice>(pool[{TypeKind::U8, types::mut::CONSTANT}], true);

    inject_function(bis::ALIGN_CAST, BP{t_type, t_auto}, t_auto);
    inject_function(bis::PTR_CAST, BP{t_type, t_auto}, t_auto);
    inject_function(bis::BIT_CAST, BP{t_type, t_auto}, t_auto);
    inject_function(bis::CONST_CAST, BP{t_auto}, t_auto);
    inject_function(bis::VOLATILE_CAST, BP{t_auto}, t_auto);
    inject_function(bis::AS, BP{t_type, t_auto}, t_auto);

    inject_function(bis::INT_FROM_PTR, BP{t_auto}, t_usize);
    inject_function(bis::PTR_FROM_INT, BP{t_type, t_usize}, t_auto);
    inject_function(bis::PTR_FROM_ARRAY, BP{t_auto}, t_auto);
    inject_function(bis::SLICE_FROM_PTR, BP{t_auto, t_usize}, t_auto);

    inject_function(bis::ALIGN_OF, BP{t_auto}, t_usize);
    inject_function(bis::SIZE_OF, BP{t_auto}, t_usize);
    inject_function(bis::TYPE_OF, BP{t_auto}, t_type);
    inject_function(bis::THIS, BP{}, t_type);
    inject_function(bis::TAG_NAME, BP{t_auto}, t_c_str);

    inject_function(bis::MEMCPY, BP{t_auto, t_auto}, t_void);
    inject_function(bis::MEMSET, BP{t_auto, t_auto}, t_void);
    inject_function(bis::MEMMOVE, BP{t_auto, t_auto}, t_void);

    inject_function(bis::MUL_ADD, BP{t_type, t_auto, t_auto, t_auto}, t_auto);
    inject_function(bis::CLZ, BP{t_auto}, t_usize);
    inject_function(bis::CTZ, BP{t_auto}, t_usize);
    inject_function(bis::POP_COUNT, BP{t_auto}, t_usize);
    inject_function(bis::SQRT, BP{t_auto}, t_auto);
    inject_function(bis::SIN, BP{t_auto}, t_auto);
    inject_function(bis::COS, BP{t_auto}, t_auto);
    inject_function(bis::TAN, BP{t_auto}, t_auto);
    inject_function(bis::EXP, BP{t_auto}, t_auto);
    inject_function(bis::EXP2, BP{t_auto}, t_auto);
    inject_function(bis::LOG, BP{t_auto}, t_auto);
    inject_function(bis::LOG2, BP{t_auto}, t_auto);
    inject_function(bis::LOG10, BP{t_auto}, t_auto);
    inject_function(bis::ABS, BP{t_auto}, t_auto);
    inject_function(bis::FLOOR, BP{t_auto}, t_auto);
    inject_function(bis::CEIL, BP{t_auto}, t_auto);

    inject_function(bis::PANIC, BP{t_c_str}, t_noreturn);
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
    auto& type = pool[{kind, types::mut::CONSTANT}];
    ASSERT(type.has_resolved(), "Builtin type was not already resolved");
    return type;
}

} // namespace ghoti::sema
