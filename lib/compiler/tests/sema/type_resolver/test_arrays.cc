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

#include "option.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

TEST_CASE("Array resolution with explicit type") {
    auto [ctx, idx] = helpers::resolve_and_check("const a: [2]i32 = [2]i32{1, 2, };");
    const auto [sym, sym_data, node_data, type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("a", idx);

    // The explicit type in the decl can't have a true size at this point
    const auto& item_type  = ctx->get_type(sema::TypeKind::I32);
    const auto& array_type = ctx->get_type(sema::TypeKind::ARRAY, false, opt::Size{}, item_type);
    CHECK(type == array_type);
    const auto& ident_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(node_data.name));
    CHECK(ident_type == array_type);
    const auto& et_type =
        helpers::unwrap(ctx->root_mod->get_sema_type_opt(*node_data.explicit_type));
    CHECK(et_type == array_type);

    // The actual value type is properly typed
    const auto& array_literal_type = ctx->get_type(sema::TypeKind::ARRAY, false, 2, item_type);
    const auto& value_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(*node_data.value));
    CHECK(value_type == array_literal_type);

    const auto& type_data = helpers::unwrap(array_literal_type.as_opt<sema::types::Array>());
    CHECK(type_data.underlying == item_type);
    CHECK(type_data.len == 2);
    CHECK_FALSE(type_data.null_terminated);

    // This is not generally true but happens to be here
    for (const auto& arr =
             helpers::unwrap(ctx->root_mod->ast.get_as_opt<ast::ArrayExpression>(*node_data.value));
         const auto item : arr.items) {
        CHECK(item_type == ctx->root_mod->get_sema_type(item));
    }
}

TEST_CASE("Array resolution with implicit type") {
    auto [ctx, idx] = helpers::resolve_and_check("const a := [4]u64{1, 2, 3, 4 };");
    const auto [sym, sym_data, node_data, type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("a", idx);

    const auto& item_type          = ctx->get_type(sema::TypeKind::U64);
    const auto& array_literal_type = ctx->get_type(sema::TypeKind::ARRAY, false, 4, item_type);
    CHECK(type == array_literal_type);
    const auto& ident_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(node_data.name));
    CHECK(ident_type == array_literal_type);
    const auto& value_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(*node_data.value));
    CHECK(value_type == array_literal_type);
}

TEST_CASE("Indexing with single accessors") {
    const auto test_index = [](std::string_view type_mod) {
        auto [ctx, idx] =
            helpers::resolve_and_check(fmt::format("var a: {}u32; const b := a[0];", type_mod));
        const auto [sym, sym_data, type] = ctx->get_type_sym_info<syms::Node>("b", idx);
        CHECK(type == ctx->get_type(sema::TypeKind::U32));
    };

    test_index("[]");
    test_index("[3]");
    test_index("*");
}

TEST_CASE("Indexing with slice accessor") {
    auto [ctx, idx] = helpers::resolve_and_check("var a: *u32; const b := a[0..4];");
    const auto [sym, sym_data, type] = ctx->get_type_sym_info<syms::Node>("b", idx);

    const auto& i32_type   = ctx->get_type(sema::TypeKind::U32);
    const auto& slice_type = ctx->get_type(sema::TypeKind::SLICE, false, i32_type);
    CHECK(type == slice_type);
}

TEST_CASE("Illegal index target") {
    auto [ctx, idx] = helpers::test_resolver_fail(
        "var a: u32; const b := a[0];",
        sema::Diagnostic{"Can only index slices, arrays, and pointers; found 'u32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 23uz}});
    ctx->check_poisoned<syms::Node>("b", idx);
}

} // namespace ghoti::tests
