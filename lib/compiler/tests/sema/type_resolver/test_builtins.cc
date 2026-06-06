#include <array>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

namespace {

// Checks for the builtin's presence in the prelude table and its return type when called
auto test_builtin_resolve(const syntax::Builtin& builtin,
                          std::string_view       mock_params,
                          auto&&                 expected_type_fn,
                          std::string_view       prelude = "") -> void {
    auto [ctx, idx] = helpers::resolve_and_check(
        fmt::format("{}const foo := {}({});", prelude, builtin.name, mock_params));

    // Quickly check to make sure the prelude table has the builtin
    const auto [builtin_sym, builtin_sym_data] = ctx->get_symbol<syms::Builtin>(
        builtin.name, helpers::unwrap(ctx->analyzer.get_prelude_index_opt()));
    CHECK(builtin_sym.get_kind_opt() == sema::SymbolKind::CALLABLE);
    const auto& builtin_type = builtin_sym_data.get_type();
    CHECK(builtin_type.get_data().as_opt<sema::types::BuiltinFunction>());

    // Now validate the actual declaration and call
    const sema::Type& expected_type = expected_type_fn(*ctx);
    const auto [decl_sym, decl_sym_data, decl_node_data, decl_type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("foo", idx);
    CHECK(expected_type == decl_type);

    const auto& call =
        helpers::unwrap(ctx->root_mod.ast.get_as_opt<ast::CallExpression>(*decl_node_data.value));
    const auto& call_type = helpers::unwrap(ctx->root_mod.get_sema_type_opt(call.function));
    CHECK(builtin_type == call_type);
}

} // namespace

namespace bis = syntax::builtins;

TEST_CASE("Builtin 'safe' casts") {
    constexpr std::array safe_casts{bis::ALIGN_CAST, bis::PTR_CAST, bis::BIT_CAST, bis::AS};
    for (const auto& bi : safe_casts) {
        test_builtin_resolve(bi, "i32, 23uz", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::I32);
        });
    }
}

TEST_CASE("Builtin 'unsafe' casts") {
    test_builtin_resolve(bis::CONST_CAST, "^23", [](helpers::SemaTestContext& ctx) -> sema::Type& {
        return ctx.get_type<sema::types::mut::MUTABLE>(sema::TypeKind::POINTER,
                                                       ctx.get_type(sema::TypeKind::I32));
    });

    test_builtin_resolve(
        bis::VOLATILE_CAST,
        "v",
        [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::I32);
        },
        "const v: volatile i32 = 23;");
}

TEST_CASE("Builtin bit/byte operations") {
    constexpr std::array ops{
        bis::ALIGN_OF, bis::SIZE_OF, bis::CLZ, bis::CTZ, bis::POP_COUNT, bis::INT_FROM_PTR};
    for (const auto& bi : ops) {
        test_builtin_resolve(bi, "123", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::USIZE);
        });
    }
}

TEST_CASE("Builtin type introspection") {
    test_builtin_resolve(bis::TYPE_OF, "i32", [](helpers::SemaTestContext& ctx) -> sema::Type& {
        return ctx.get_type(sema::TypeKind::TYPE, ctx.get_type(sema::TypeKind::I32));
    });

    test_builtin_resolve(bis::TAG_NAME, "123", [](helpers::SemaTestContext& ctx) -> sema::Type& {
        return ctx.get_type(sema::TypeKind::SLICE, true, ctx.get_type(sema::TypeKind::U8));
    });
}

TEST_CASE("Builtin this introspection") {
    auto [ctx, idx]              = helpers::resolve_and_check("struct { using A = @this(); };");
    const auto [sym, data, type] = ctx->get_type_sym_info<syms::Node>("A", idx + 1);
    CHECK(type == ctx->get_type(sema::TypeKind::STRUCT, idx + 1));
}

TEST_CASE("Deferred return type from typeOf") {
    auto [ctx, idx] =
        helpers::resolve_and_check("const a := fn(): type {}; using B = @typeOf(a());");

    const auto [sym, sym_data, node_data, type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::UsingStatement>("B", idx);

    const auto& call =
        helpers::unwrap(ctx->root_mod.ast.get_as_opt<ast::CallExpression>(node_data.explicit_type));
    CHECK(type == ctx->get_type(sema::TypeKind::TYPE, &call));
    CHECK(&call == &helpers::unwrap(type.get_data().as_opt<sema::types::DeferredCall>()).call);
}

TEST_CASE("Builtin pointer conversions") {
    test_builtin_resolve(
        bis::PTR_FROM_ARRAY,
        "a",
        [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::POINTER, ctx.get_type(sema::TypeKind::I32));
        },
        "var a := [_]i32{0, 1, 2};");

    test_builtin_resolve(
        bis::PTR_FROM_INT, "^i32, 0xc0ffeeul", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::POINTER, ctx.get_type(sema::TypeKind::I32));
        });

    test_builtin_resolve(
        bis::SLICE_FROM_PTR, "^1, 20uz", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::SLICE, false, ctx.get_type(sema::TypeKind::I32));
        });
}

TEST_CASE("Builtins memory operation") {
    constexpr std::array ops{bis::MEMCPY, bis::MEMSET, bis::MEMMOVE};
    for (const auto& bi : ops) {
        test_builtin_resolve(
            bi,
            "a, b",
            [](helpers::SemaTestContext& ctx) -> sema::Type& {
                return ctx.get_type(sema::TypeKind::VOID);
            },
            "var a: i32; var b: i32;");
    }
}

TEST_CASE("Builtin arithmetic") {
    constexpr std::array fns{bis::SQRT,
                             bis::SIN,
                             bis::COS,
                             bis::TAN,
                             bis::EXP,
                             bis::EXP2,
                             bis::LOG,
                             bis::LOG2,
                             bis::LOG10,
                             bis::ABS,
                             bis::FLOOR,
                             bis::CEIL};
    for (const auto& bi : fns) {
        test_builtin_resolve(bi, "2.34f", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::F32);
        });
    }

    test_builtin_resolve(
        bis::MUL_ADD, "f64, 1.0, 2.0, 3.0", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::F64);
        });
}

TEST_CASE("Builtin control flow") {
    test_builtin_resolve(
        bis::PANIC, R"("Help!")", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::NORETURN);
        });
}

TEST_CASE("Builtin function arity mismatch") {
    helpers::test_resolver_fail("const foo := @sizeOf();",
                                sema::Diagnostic{"Builtin expects 1 arguments, found 0",
                                                 sema::Error::ARITY_MISMATCH,
                                                 std::pair{0uz, 13uz}});
}

TEST_CASE("Const cast quick type checking") {
    helpers::test_resolver_fail("const foo := @constCast(1);",
                                sema::Diagnostic{"Expected pointer or reference type; found 'i32'",
                                                 sema::Error::TYPE_MISMATCH,
                                                 std::pair{0uz, 24uz}});
}

TEST_CASE("Other builtin quick type mismatch") {
    helpers::test_resolver_fail(
        "const foo := @ptrFromArray(1);",
        sema::Diagnostic{"Expected an array-yielding expression; found 'i32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 27uz}});

    helpers::test_resolver_fail("const foo := @ptrFromInt(i32, 0xdeadbeeful);",
                                sema::Diagnostic{"Expected a pointer type; found 'i32'",
                                                 sema::Error::TYPE_MISMATCH,
                                                 std::pair{0uz, 25uz}});

    helpers::test_resolver_fail(
        "const foo := @sliceFromPtr(1, 20uz);",
        sema::Diagnostic{"Expected a pointer-yielding expression; found 'i32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 27uz}});
}

TEST_CASE("Illegal @this usage") {
    helpers::test_resolver_fail(
        "@this();",
        sema::Diagnostic{"@this() may only be used inside of structs, unions, and enums",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 5uz}});
}

} // namespace ghoti::tests
