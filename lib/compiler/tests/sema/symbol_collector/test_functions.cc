#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

TEST_CASE("Function hollow types") {
    auto [ctx, idx] =
        helpers::collect_and_check("const a := fn(&self, c: type): void { const foo := bar; };");
    const auto& registry = ctx->analyzer.get_registry();
    REQUIRE(registry.size() == 2);

    const auto [sym, sym_data, node_data] =
        ctx->get_ast_sym_info<syms::Node, ast::DeclStatement>("a", idx);
    CHECK(sym.get_kind_opt() == sema::SymbolKind::CALLABLE);

    const auto& fn_type = helpers::unwrap(ctx->root_mod.get_sema_type_opt(*node_data.value));
    CHECK(fn_type == ctx->get_type(sema::TypeKind::FUNCTION, 1));

    REQUIRE(
        helpers::unwrap(registry.get_from_opt(1, "self")).get_data().as_opt<syms::SelfParameter>());
    REQUIRE(helpers::unwrap(registry.get_from_opt(1, "c")).get_data().as_opt<syms::Parameter>());
    ctx->test_common_decl_collection(1);
}

TEST_CASE("Well-placed function control-flow statements") {
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): void { return; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { return 0; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { defer a = 2; };");
}

TEST_CASE("Constexpr function declaration") {
    helpers::collect_and_check("pub constexpr work := fn(): i32 { return 1; };");
}

TEST_CASE("Defer statements respect identifier collection rules") {
    helpers::test_collector_fail(
        "pub const main := fn(args: [][:0]u8): i32 { defer { var main: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'main'; previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 52uz}});
}

TEST_CASE("Function basic param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f: bool): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'; previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function self param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'; previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function local param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(a, a: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'; previous declaration here: 1:15",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 17uz}});
}

TEST_CASE("Function block shadowing") {
    helpers::test_collector_fail(
        "const f := fn(): void { var f := 3; };",
        sema::Diagnostic{"Attempt to shadow identifier 'f'; previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 24uz}});
}

} // namespace ghoti::tests
