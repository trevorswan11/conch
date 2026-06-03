#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/expression.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "option.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

TEST_CASE("For loop resolution") {
    auto [ctx, idx] = helpers::resolve_and_check(R"(
        var arr: []bool;
        const a := outer: for (0..5, blk: { break :blk 2..4; }, arr) |i, j, _| {
            const foo := 39 + j;
            if (foo == 42) { break :outer 27; }
        } else {
            const foo := 42;
            if (foo + 1 == 42) { break :outer 26; }
        };
    )");
    CHECK(ctx->analyzer.get_registry().size() == 9);

    const auto& i32_type                   = ctx->get_type(sema::TypeKind::I32);
    const auto [a_sym, a_sym_data, a_type] = ctx->get_type_sym_info<syms::Node>("a", idx);
    CHECK(a_type == i32_type);

    {
        const auto [sym, sym_data, node_data, type] =
            ctx->get_ast_type_sym_info<syms::Label, ast::LabelExpression>(
                "outer", idx + 1, opt::none, &syms::Label::get_definition);
        CHECK(ctx->root_mod->get_sema_type(node_data.name) == i32_type);
    }

    const auto check_capture = [&](std::string_view name) {
        const auto [sym, sym_data, type] = ctx->get_type_sym_info<syms::ForLoopCapture>(
            name, 4, opt::none, &syms::ForLoopCapture::payload);
        CHECK(type == i32_type);
    };
    check_capture("i");
    check_capture("j");
}

TEST_CASE("Trivial loop resolution") {
    helpers::resolve_and_check(
        "const a := do { const foo := 42; } while (blk: { break :blk 42; });");
    helpers::resolve_and_check("const a := loop { const foo := 42; };");
    helpers::resolve_and_check(R"(
        var i: i32;
        const a := outer: while (blk: { break :blk 42; }) : (i += blk: {
            break :blk 42;
        }) {
            break :outer if (i == 42) 3 else blk: {
                const foo := 42;
                break :blk 42;
            };
        } else { const foo := 42; };
    )");
}

TEST_CASE("Illegal for loop iterable type") {
    helpers::test_resolver_fail(
        "for (23) |_| { var a: i32; }",
        sema::Diagnostic{"Iterables may only be arrays or slices; found 'i32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 5uz}});
}

} // namespace ghoti::tests
