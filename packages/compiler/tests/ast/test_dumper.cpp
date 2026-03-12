#include <sstream>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

namespace conch::tests {

constexpr std::string_view input{R"(
    [_]*N{a, b, c, d, e, 3, "54" };
    a <= b or c == d and e;
    a or b[3uz] == !c;
    continue;
    return enum { RED };
    import std;
    import "ast/node.conch" as node;
    _ = enum { RED };
    comptime SIZE := 2uz;
    { a; b; 2; c; };
    while (true) : (i += 1) {a;} else return b;
    var f_ptr: *fn(&a, b: *mut B): &[0x2uz][N]*E;
    A::B::C;
    packed struct { var a: Foo = bar; const b := fn(*mut this, a: A, b: *B): C { c; }; };
    &a; &mut b; *a;
    match (a) { b => |c| d; e => |_| f; g => h; } else d;
    loop { a; };
    (*arr[i][j]) = 2;
    if (a) { b; } else { c; };
    fn(*mut this, a: A, b: *B, ): int { c; };
    for (arr, l, p) |i, &mut j, _| { a; } else return b;
    enum : ulong {A = 1ul, B = T, C, };
    @ptrAdd(a, 4uz);
    using T = int;
    a(&mut r, t, *[N]int);
    a->b;
    .a;
    union { a: int, b: &mut T, };
)"};

constexpr std::string_view expected{
#include "dump.inc"
};

TEST_CASE("Comprehensive dump") {
    Parser p{input};
    auto [ast, errors] = p.consume();
    helpers::check_errors<ParserDiagnostic>(errors);

    std::ostringstream oss;
    ast::ASTDumper     dumper{oss};
    for (const auto& node : ast) { node->accept(dumper); }
    REQUIRE(expected == oss.view());
}

} // namespace conch::tests
