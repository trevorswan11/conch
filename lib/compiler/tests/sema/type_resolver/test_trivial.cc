#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fmt/format.h"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

namespace ghoti::tests {

namespace syms = sema::symbols;

TEST_CASE("Builtin type resolution") {
    const auto check_bi_type = [](std::string_view value, sema::TypeKind expected_kind) {
        auto [ctx, idx] = helpers::resolve_and_check(fmt::format("const a := {};", value));
        const auto [sym, data, type] = ctx->get_type_sym_info<syms::Node>("a", idx);
        CHECK(type == ctx->get_type(expected_kind));
    };

    using TK = sema::TypeKind;
    check_bi_type("1", TK::I32);
    check_bi_type("1l", TK::I64);
    check_bi_type("1z", TK::ISIZE);
    check_bi_type("1u", TK::U32);
    check_bi_type("1ul", TK::U64);
    check_bi_type("1uz", TK::USIZE);
    check_bi_type("'1'", TK::U8);
    check_bi_type("true", TK::BOOL);
    check_bi_type("{}", TK::VOID);
    check_bi_type("undefined", TK::UNDEFINED);
    check_bi_type("1.0f", TK::F32);
    check_bi_type("1.0", TK::F64);
}

TEST_CASE("Nested type resolution") {
    auto [ctx, idx] = helpers::resolve_and_check("var a: **i32; var b: ***i32;");

    const auto& i32_ptr =
        ctx->get_type(sema::TypeKind::POINTER, ctx->get_type(sema::TypeKind::I32));
    const auto& i32_ptr_ptr = ctx->get_type(sema::TypeKind::POINTER, i32_ptr);

    const auto [a_sym, a_sym_data, a_type] = ctx->get_type_sym_info<syms::Node>("a", idx);
    CHECK(a_type == i32_ptr_ptr);
    const auto [b_sym, b_sym_data, b_type] = ctx->get_type_sym_info<syms::Node>("b", idx);
    CHECK(b_type == ctx->get_type(sema::TypeKind::POINTER, i32_ptr_ptr));
}

TEST_CASE("Type alias resolution") {
    auto [ctx, idx] = helpers::resolve_and_check("using a = *bool; var b: a; var c: &a;");
    const auto& bool_ref =
        ctx->get_type(sema::TypeKind::POINTER, ctx->get_type(sema::TypeKind::BOOL));

    const auto [a_sym, a_sym_data, a_type] = ctx->get_type_sym_info<syms::Node>("a", idx);
    CHECK(a_type == bool_ref);
    const auto [b_sym, b_sym_data, b_type] = ctx->get_type_sym_info<syms::Node>("b", idx);
    CHECK(b_type == bool_ref);
    const auto [c_sym, c_sym_data, c_type] = ctx->get_type_sym_info<syms::Node>("c", idx);
    CHECK(c_type == ctx->get_type(sema::TypeKind::REFERENCE, bool_ref));
}

TEST_CASE("Undeclared identifier usage") {
    auto [ctx, idx] = helpers::test_resolver_fail("const a := b;",
                                                  sema::Diagnostic{
                                                      "Use of undeclared identifier 'b'",
                                                      sema::Error::UNDECLARED_IDENTIFIER,
                                                      std::pair{0uz, 11uz},
                                                  });
    ctx->check_poisoned<syms::Node>("a", idx);
}

TEST_CASE("Duplicate test name") {
    helpers::test_resolver_fail(
        R"(test "TEST ME" { var a: i32; } test "TEST ME" { var a: i32; })",
        sema::Diagnostic{"Duplicate test block named 'TEST ME'; previous declaration here: 1:1",
                         sema::Error::DUPLICATE_TEST_NAME,
                         std::pair{0uz, 31uz}});
}

} // namespace ghoti::tests
