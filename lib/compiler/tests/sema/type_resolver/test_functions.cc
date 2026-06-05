#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "option.hh"
#include "types.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

TEST_CASE("Function declaration and call type resolution") {
    auto [ctx, idx] = helpers::resolve_and_check(R"(
        const a := fn(b: i32, c: *@typeOf(b), d: [:0]u8): bool {
            return true;
        };

        const int: i32 = 12;
        const result := a(1, ^int, "Hello, World");
    )");

    const auto& i32_type  = ctx->get_type(sema::TypeKind::I32);
    const auto& bool_type = ctx->get_type(sema::TypeKind::BOOL);

    const auto [a_decl_sym, a_decl_sym_data, a_decl_node_data, a_decl_type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("a", idx);

    SECTION("Function validation") {
        CHECK(a_decl_sym.get_kind_opt() == sema::SymbolKind::CALLABLE);

        const auto check_param_type = [&](std::string_view name, const sema::Type& expected_type) {
            const auto [sym, sym_data, type] =
                ctx->get_type_sym_info<syms::Parameter>(name, 1, opt::none, &syms::Parameter::name);
            CHECK(sym.get_kind_opt() == sema::SymbolKind::VALUE);
            CHECK(type == expected_type);
        };

        check_param_type("b", i32_type);

        const auto& c_meta = ctx->get_type(sema::TypeKind::TYPE, i32_type);
        const auto& c_ptr  = ctx->get_type(sema::TypeKind::POINTER, c_meta);
        check_param_type("c", c_ptr);
        const auto& c_meta_data = helpers::unwrap(c_meta.as_opt<sema::types::MetaType>());
        CHECK(c_meta_data.instance == i32_type);

        const auto& u8_slice =
            ctx->get_type(sema::TypeKind::SLICE, true, ctx->get_type(sema::TypeKind::U8));
        check_param_type("d", u8_slice);

        const auto& fn = helpers::unwrap(
            ctx->root_mod->ast.get_as_opt<ast::FunctionExpression>(*a_decl_node_data.value));
        const auto& return_type = ctx->root_mod->get_sema_type(fn.explicit_return_type);
        CHECK(return_type == bool_type);
    }

    SECTION("Call validation") {
        const auto [decl_sym, decl_sym_data, decl_node_data, decl_type] =
            ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("result", idx);
        CHECK(decl_sym.get_kind_opt() == sema::SymbolKind::VALUE);
        CHECK(decl_type == bool_type);

        const auto& call = helpers::unwrap(
            ctx->root_mod->ast.get_as_opt<ast::CallExpression>(*decl_node_data.value));
        CHECK(call.arguments.size() == 3);
        const auto& call_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(call.function));
        CHECK(a_decl_type == call_type);

        const auto check_arg_type = [&](usize arg_idx, const sema::Type& expected_type) {
            REQUIRE(arg_idx < call.arguments.size());
            const auto arg =
                helpers::unwrap(call.arguments[arg_idx].as_opt<ast::ExpressionHandle>());
            const auto& arg_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(arg));
            CHECK(expected_type == arg_type);
        };

        check_arg_type(0, i32_type);
        check_arg_type(1, ctx->get_type(sema::TypeKind::POINTER, i32_type));

        const auto last_arg = helpers::unwrap(call.arguments[2].as_opt<ast::ExpressionHandle>());
        const auto str_size = ctx->get_string_literal_size(last_arg);

        const auto& u8_type = ctx->get_type(sema::TypeKind::U8);
        const auto& null_string_type =
            ctx->get_type(sema::TypeKind::ARRAY, true, str_size, u8_type);

        const auto& last_arg_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(last_arg));
        CHECK(null_string_type == last_arg_type);
    }
}

TEST_CASE("Self parameters in structural types") {
    // Assumes the type has a member function called foo that takes self by reference
    const auto check_structural_type = [](std::string_view input, sema::TypeKind self_kind) {
        auto [ctx, idx] = helpers::resolve_and_check(input);

        const auto [a_decl_sym, a_decl_sym_data, a_decl_node_data, a_decl_type] =
            ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("a", idx);
        CHECK(a_decl_sym.get_kind_opt() == sema::SymbolKind::TYPE);
        const auto struct_idx = helpers::unwrap(a_decl_type.get_symbol_table_idx_opt(), 1uz);

        const auto [fn_sym, fn_sym_data, fn_node_data, fn_type, fn_type_data] =
            ctx->get_full_sym_info<syms::Node, ast::DeclStatement, sema::types::Function>(
                "foo", struct_idx);
        REQUIRE(fn_type_data.params.size() == 1);
        const auto& expected_param_type = ctx->get_type(self_kind, struct_idx, a_decl_type);
        CHECK(expected_param_type == *fn_type_data.params[0]);
    };

    check_structural_type(R"(const a := struct {
        const foo := fn(&self): void {};
    };)",
                          sema::TypeKind::REFERENCE);

    check_structural_type(R"(const a := enum {
        b,
        const foo := fn(&self): void {};
    };)",
                          sema::TypeKind::REFERENCE);

    check_structural_type(R"(const a := union {
        b: i32,
        const foo := fn(*self): void {};
    };)",
                          sema::TypeKind::POINTER);
}

TEST_CASE("Deferred return type from user function") {
    auto [ctx, idx] = helpers::resolve_and_check("const a := fn(): type {}; using B = a();");

    const auto [sym, sym_data, node_data, type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::UsingStatement>("B", idx);

    const auto& call = helpers::unwrap(
        ctx->root_mod->ast.get_as_opt<ast::CallExpression>(node_data.explicit_type));
    CHECK(type == ctx->get_type(sema::TypeKind::TYPE, &call));
    CHECK(&call == &helpers::unwrap(type.as_opt<sema::types::DeferredEval>()).call_node);
}

TEST_CASE("Function explicit type resolution") {
    auto [ctx, idx]              = helpers::resolve_and_check("var foo: fn(*i32, u32): bool;");
    const auto [sym, data, type] = ctx->get_type_sym_info<syms::Node>("foo", idx);

    const auto& expected_type =
        ctx->get_type(sema::TypeKind::FUNCTION,
                      ctx->get_type(sema::TypeKind::POINTER, ctx->get_type(sema::TypeKind::I32)),
                      ctx->get_type(sema::TypeKind::U32),
                      ctx->get_type(sema::TypeKind::BOOL));
    CHECK(type == expected_type);
}

TEST_CASE("Function with syntactically ambiguous arguments") {
    auto [ctx, idx] = helpers::resolve_and_check(R"(
        using a = @typeOf(*i32);
        using b = @typeOf(*mut i32);
        using c = @typeOf(&i32);
        using d = @typeOf(&mut i32);
        using e = @typeOf(**i32);
        using f = @typeOf(*mut *i32);
    )");

    const auto check_ambiguous = [&](std::string_view name, const sema::Type& instance_type) {
        const auto& meta_type     = ctx->get_type(sema::TypeKind::TYPE, instance_type);
        const auto [sym, _, type, data] = ctx->get_full_type_sym_info<syms::Node, sema::types::MetaType>(name, idx);
        fmt::println("{}", magic_enum::enum_name(data.instance.get_kind()));
        CHECK(type == meta_type);
    };

    namespace mut             = sema::types::mut;
    const auto& i32_type      = ctx->get_type(sema::TypeKind::I32);
    const auto& i32_const_ptr = ctx->get_type(sema::TypeKind::POINTER, i32_type);

    check_ambiguous("a", i32_const_ptr);
    check_ambiguous("b", ctx->get_type<mut::MUTABLE>(sema::TypeKind::POINTER, i32_type));
    check_ambiguous("c", ctx->get_type(sema::TypeKind::REFERENCE, i32_type));
    check_ambiguous("d", ctx->get_type<mut::MUTABLE>(sema::TypeKind::REFERENCE, i32_type));
    check_ambiguous("e", ctx->get_type(sema::TypeKind::POINTER, i32_const_ptr));
    check_ambiguous("f", ctx->get_type<mut::MUTABLE>(sema::TypeKind::POINTER, i32_const_ptr));
}

TEST_CASE("Self parameters in non-structural types") {
    auto [ctx, idx] = helpers::test_resolver_fail(
        "const foo := fn(&self): void {};",
        sema::Diagnostic{"Self parameters may only be used inside member functions",
                         sema::Error::ILLEGAL_SELF_PARAMETER,
                         std::pair{0uz, 17uz}});
    ctx->check_poisoned<syms::Node>("foo", idx);
}

TEST_CASE("Declared function arity mismatch") {
    auto [ctx, idx] = helpers::test_resolver_fail(
        "const foo := fn(a: i32, b: i32): void {}; const bar := foo(1);",
        sema::Diagnostic{
            "Expected 2 arguments, found 1", sema::Error::ARITY_MISMATCH, std::pair{0uz, 55uz}});
    ctx->check_poisoned<syms::Node>("bar", idx);
}

TEST_CASE("Non-callable expression") {
    auto [ctx, idx] =
        helpers::test_resolver_fail("const bar := 5; const foo := bar();",
                                    sema::Diagnostic{"Expression is not callable",
                                                     sema::Error::NON_CALLABLE_EXPRESSION,
                                                     std::pair{0uz, 29uz}});
    ctx->check_poisoned<syms::Node>("foo", idx);
}

} // namespace ghoti::tests
