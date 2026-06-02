#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "helpers/common.hh"
#include "helpers/sema.hh"

namespace porpoise::tests {

using helpers::MockFile;

namespace {

constexpr std::string_view other_porp{R"(
pub const foo := fn(c: u8): void {};

pub const BarE := enum {
    A,
    pub const bar := fn(&self, c: u8): void {};
};

pub const BarU := union {
    A: i32,
    pub const bar := fn(&self, c: u8): void {};
};

pub const BarS := struct {
    pub var baz: i32 = 23;
    pub const bar := fn(&self, c: u8): void {};
};
)"};

[[nodiscard]] auto setup_access_test(std::string_view input) -> helpers::CtxIdxPair {
    return helpers::resolve_and_check(
        fmt::format(R"(import "other.porp" as other; {})", input),
        helpers::make_vector<MockFile>(MockFile{"other.porp", other_porp}));
}

} // namespace

TEST_CASE("Free function resolved access") {
    auto [ctx, idx] = setup_access_test("const a := other::foo('a');");
}

TEST_CASE("Enum resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var bar_e: other::BarE = .A;
const e1 := other::BarE.A;
const e2 := bar_e.bar('a');
)");
}

TEST_CASE("Union resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var bar_u: other::BarU = .{ .A = 1, };
const u1 := other::BarU{ .A = 1, };
const u2 := bar_u.bar('a');
)");
}

TEST_CASE("Struct resolved access") {
    auto [ctx, idx] = setup_access_test(R"(
var bar_s: other::BarS = .{};
const s1 := bar_s.baz;
const s2 := bar_s.bar('a');
)");
}

} // namespace porpoise::tests
