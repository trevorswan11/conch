#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "types.hh"

namespace porpoise::tests {

using helpers::MockFile;
namespace syms = sema::symbols;

namespace {

constexpr std::string_view other_porp{R"(
pub const foo := fn(c: u8): []u8 {};

pub const BarE := enum {
    A,
    pub const bar := fn(&self, c: u8): []u8 {};
};

pub const BarU := union {
    A: i32,
    pub const bar := fn(&self, c: u8): []u8 {};
};

pub const BarS := struct {
    pub var baz: i32 = 23;
    pub const bar := fn(&self, c: u8): []u8 {};
};
)"};

[[nodiscard]] auto setup_access_test(std::string_view input) -> helpers::CtxIdxPair {
    return helpers::resolve_and_check(
        fmt::format(R"(import "other.porp" as other; {})", input),
        helpers::make_vector<MockFile>(MockFile{"other.porp", other_porp}));
}

[[nodiscard]] auto u8_slice_type(helpers::SemaTestContext& ctx) -> sema::Type& {
    return ctx.get_type(sema::TypeKind::SLICE, false, ctx.get_type(sema::TypeKind::U8));
}

[[nodiscard]] auto bar_fn_type(helpers::SemaTestContext& ctx, usize table_idx) -> sema::Type& {
    return ctx.get_type(sema::TypeKind::FUNCTION, table_idx);
}

auto check_access_decl(helpers::SemaTestContext& ctx,
                       usize                     idx,
                       std::string_view          symbol_name,
                       const sema::Type&         expected_type) -> void {
    const auto [sym, data, type] = ctx.get_type_sym_info<syms::Node>(symbol_name, idx);
    CHECK(expected_type == type);
}

} // namespace

TEST_CASE("Free function resolved access") {
    auto [ctx, idx] = setup_access_test("const a := other::foo('a');");
    check_access_decl(*ctx, idx, "a", u8_slice_type(*ctx));
}

TEST_CASE("Enum resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var e1: other::BarE = .A;
const e2 := other::BarE.A;
const e3 := e1.bar('a');

const func := other::BarE.bar;
)");

    const auto& enum_type = ctx->get_type(sema::TypeKind::ENUM, 3);
    check_access_decl(*ctx, idx, "e1", enum_type);
    check_access_decl(*ctx, idx, "e2", enum_type);
    check_access_decl(*ctx, idx, "e3", u8_slice_type(*ctx));
    check_access_decl(*ctx, idx, "func", bar_fn_type(*ctx, 4));
}

TEST_CASE("Union resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var u1: other::BarU = .{ .A = 1, };
const u2 := other::BarU{ .A = 1, };
const u3 := u1.bar('a');

const func := other::BarU.bar;
)");

    const auto& union_type = ctx->get_type(sema::TypeKind::UNION, 5);
    check_access_decl(*ctx, idx, "u1", union_type);
    check_access_decl(*ctx, idx, "u2", union_type);
    check_access_decl(*ctx, idx, "u3", u8_slice_type(*ctx));
    check_access_decl(*ctx, idx, "func", bar_fn_type(*ctx, 6));
}

TEST_CASE("Struct resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var s1: other::BarS = .{ .baz = 42, };
const s2 := s1.baz;
const s3 := s1.bar('a');

const member := other::BarS.baz;
const func := other::BarS.bar;
)");

    const auto& struct_type = ctx->get_type(sema::TypeKind::STRUCT, 7);
    const auto& member_type = ctx->get_type(sema::TypeKind::I32);

    check_access_decl(*ctx, idx, "s1", struct_type);
    check_access_decl(*ctx, idx, "s2", member_type);
    check_access_decl(*ctx, idx, "s3", u8_slice_type(*ctx));
    check_access_decl(*ctx, idx, "member", member_type);
    check_access_decl(*ctx, idx, "func", bar_fn_type(*ctx, 8));
}

} // namespace porpoise::tests
