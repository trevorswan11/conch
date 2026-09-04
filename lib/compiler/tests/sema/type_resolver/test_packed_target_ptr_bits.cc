#include <catch2/catch_test_macros.hpp>

#include "compiler/sema/error.hh"
#include "helpers/sema.hh"

namespace ghoti::tests {

// Two pointer-like fields plus a 1-bit tail: with the target's real pointer width (32 bits on
// this triple) the backing width is 32 + 32 + 1 = 65 bits, well within the 128-bit packed-struct
// limit. A resolver that still assumed a 64-bit pointer would compute 64 + 64 + 1 = 129 bits and
// wrongly reject this as too wide.
TEST_CASE("packed struct pointer field sizing uses the real 32-bit target pointer width") {
    auto [ctx, idx]{
        helpers::resolve_for_target("const S := packed struct { p1: ^u8, p2: ^u8, tail: u1 };",
                                    "armv7-unknown-linux-gnueabihf")};
    helpers::check_errors<sema::diagnostics>(ctx->root_mod);
}

// The same struct on a 64-bit target: 64 + 64 + 1 = 129 bits legitimately exceeds the limit and
// must still be rejected.
TEST_CASE("packed struct pointer field sizing still rejects an over-wide struct on a 64-bit "
          "target") {
    auto [ctx, idx]{helpers::resolve_for_target(
        "const S := packed struct { p1: ^u8, p2: ^u8, tail: u1 };", "x86_64-unknown-linux-gnu")};
    const auto& diags{UNWRAP(ctx->root_mod.diagnostics.as_opt<sema::diagnostics>())};
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].get_error() == sema::error::ILLEGAL_PACKED_FIELD);
}

} // namespace ghoti::tests
