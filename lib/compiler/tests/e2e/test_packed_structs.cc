#include <catch2/catch_test_macros.hpp>

#include "helpers/codegen.hh"

namespace ghoti::tests {

// A non-extern `packed struct` is laid out as a single backing integer whose width
// is the sum of its fields' bit sizes, fields packed LSB-first in declaration order.

TEST_CASE("packed struct is backed by a sum-width integer") {
    CHECK(helpers::compile_and_run(R"(
        const Pair := packed struct { a: u3, b: u5 };
        pub const main := fn(): i32 {
            if (@sizeOf(Pair) != 1) { return 1; }
            var p: Pair = .{ .a = 5, .b = 5 };
            if (@as(i32, p.a) != 5) { return 2; }
            if (@as(i32, p.b) != 5) { return 3; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("packed struct fields do not bleed into each other") {
    CHECK(helpers::compile_and_run(R"(
        const Pair := packed struct { a: u3, b: u5 };
        pub const main := fn(): i32 {
            var p: Pair = .{ .a = 7, .b = 0 };
            if (@as(i32, p.a) != 7 or @as(i32, p.b) != 0) { return 1; }
            var q: Pair = .{ .a = 0, .b = 31 };
            if (@as(i32, q.a) != 0 or @as(i32, q.b) != 31) { return 2; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("packed struct field write updates only its own bits") {
    CHECK(helpers::compile_and_run(R"(
        const Pair := packed struct { a: u3, b: u5 };
        pub const main := fn(): i32 {
            var p: Pair = .{ .a = 7, .b = 31 };
            p.a = 2;
            if (@as(i32, p.a) != 2 or @as(i32, p.b) != 31) { return 1; }
            p.b = 9;
            if (@as(i32, p.a) != 2 or @as(i32, p.b) != 9) { return 2; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("packed struct LSB-first layout is observable through @bitCast") {
    CHECK(helpers::compile_and_run(R"(
        const Pair := packed struct { a: u3, b: u5 };
        pub const main := fn(): i32 {
            const p: Pair = .{ .a = 5, .b = 5 };
            // b occupies bits 3..7: 5 | (5 << 3) == 45
            return @as(i32, @bitCast(u8, p));
        };
    )") == 45);
}

TEST_CASE("packed struct wider than a machine word") {
    CHECK(helpers::compile_and_run(R"(
        const Wide := packed struct { lo: u40, hi: u40 };
        pub const main := fn(): i32 {
            if (@sizeOf(Wide) != 16) { return 1; }
            var w: Wide = .{ .lo = 1000000, .hi = 2000000 };
            if (@as(i32, @as(u64, w.lo)) != 1000000) { return 2; }
            if (@as(i32, @as(u64, w.hi)) != 2000000) { return 3; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("packed struct containing bool and enum fields") {
    CHECK(helpers::compile_and_run(R"(
        const Color := enum : u2 { Red = 0, Green = 1, Blue = 2 };
        const Flags := packed struct { on: bool, c: Color, n: u5 };
        pub const main := fn(): i32 {
            var f: Flags = .{ .on = true, .c = Color.Blue, .n = 21 };
            if (!f.on) { return 1; }
            if (f.c != Color.Blue) { return 2; }
            if (@as(i32, f.n) != 21) { return 3; }
            f.on = false;
            if (f.on or f.c != Color.Blue or @as(i32, f.n) != 21) { return 4; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("nested packed struct field round-trips") {
    CHECK(helpers::compile_and_run(R"(
        const Inner := packed struct { x: u4, y: u4 };
        const Outer := packed struct { head: u8, body: Inner };
        pub const main := fn(): i32 {
            if (@sizeOf(Outer) != 2) { return 1; }
            var o: Outer = .{ .head = 200, .body = .{ .x = 3, .y = 12 } };
            if (@as(i32, o.head) != 200) { return 2; }
            if (@as(i32, o.body.x) != 3 or @as(i32, o.body.y) != 12) { return 3; }
            return 0;
        };
    )") == 0);
}

TEST_CASE("extern packed struct keeps C field layout, not bit-packing") {
    CHECK(helpers::compile_and_run(R"(
        const E := extern packed struct { a: i32, b: u8 };
        pub const main := fn(): i32 {
            var e: E = .{ .a = 42, .b = 7 };
            return e.a + @as(i32, e.b);
        };
    )") == 49);
}

} // namespace ghoti::tests
