#include "catch_amalgamated.hpp"

#include <cstdlib>
#include <string>

#include "parser_helpers.hpp"

TEST_CASE("Declaration reconstructions") {
    SECTION("Simple") {
        test_reconstruction("var my_var := another_var", "var my_var := another_var;");
        test_reconstruction("const a: int = 2", "const a: int = 2;");
    }

    SECTION("Function types") {
        test_reconstruction("const a: fn(b: int): int = 2;", "const a: fn(b: int): int = 2;");
        test_reconstruction("var a: fn<T>(b: int): Walker", "var a: fn<T>(b: int): Walker;");
        test_reconstruction("var a: fn<T, E>(b: int): Walker", "var a: fn<T, E>(b: int): Walker;");
        test_reconstruction("var a: fn<T, E>(ref b: int): Walker",
                            "var a: fn<T, E>(ref b: int): Walker;");
    }

    SECTION("Struct types") {
        test_reconstruction("var a: struct {b: int, }", "var a: struct { b: int, };");
        test_reconstruction("var a: struct<T>{b: int, }", "var a: struct<T>{ b: int, };");
        test_reconstruction("var a: struct<T, E>{b: int, }", "var a: struct<T, E>{ b: int, };");
    }

    SECTION("Enum types") {
        test_reconstruction("var a: enum {ONE, TWO, THREE, }", "var a: enum { ONE, TWO, THREE, };");
        test_reconstruction("const a: enum { ONE, TWO, THREE, } = ONE",
                            "const a: enum { ONE, TWO, THREE, } = ONE;");
        test_reconstruction("const a: enum { ONE = 3, TWO, THREE, } = ONE",
                            "const a: enum { ONE = 3, TWO, THREE, } = ONE;");
    }

    SECTION("Array types") {
        test_reconstruction("var a: [2uz]int;", "var a: [2uz]int;");
        test_reconstruction("var a: [2uz]int = [_]{};", "var a: [2uz]int = [_]{ };");
        test_reconstruction("var a: ?[2uz]int = [_]{};", "var a: ?[2uz]int = [_]{ };");
        test_reconstruction("var a: [2uz]?int = [_]{};", "var a: [2uz]?int = [_]{ };");
        test_reconstruction("var a: [2uz, 2uz]int = [_]{};", "var a: [2uz, 2uz]int = [_]{ };");
        test_reconstruction("var a: [2uz]int = [0b10uz]{2, 3, };",
                            "var a: [2uz]int = [2uz]{ 2, 3, };");
        test_reconstruction("const a := a..3u;", "const a := a..3u;");
        test_reconstruction("const a := a..=3uz;", "const a := a..=3uz;");
        test_reconstruction("a[3uz]", "a[3uz]");
    }

    SECTION("Type declarations") {
        test_reconstruction("type a = fn(a: int): int", "type a = fn(a: int): int;");
        test_reconstruction("type a = fn<T>(a: int): int", "type a = fn<T>(a: int): int;");
        test_reconstruction("type a = fn<T, E>(a: int): int", "type a = fn<T, E>(a: int): int;");

        test_reconstruction("type a = struct { b: int, }", "type a = struct { b: int, };");
        test_reconstruction("type a = struct<T>{ b: int, }", "type a = struct<T>{ b: int, };");
        test_reconstruction("type a = struct<T, E>{ b: int, }",
                            "type a = struct<T, E>{ b: int, };");

        test_reconstruction("type Colors = enum {RED, GREEN, BLUE, }",
                            "type Colors = enum { RED, GREEN, BLUE, };");
    }

    SECTION("Typeof'd types") {
        test_reconstruction("type T = typeof 2", "type T = typeof 2;");
        test_reconstruction("type T = typeof 2u", "type T = typeof 2u;");
        test_reconstruction(R"(type T = typeof "Hello, World!")",
                            R"(type T = typeof "Hello, World!";)");
        test_reconstruction("type T = typeof AAABBB", "type T = typeof AAABBB;");
    }
}

TEST_CASE("Numbers") {
    test_reconstruction("const a := 12", "const a := 12;");
    test_reconstruction("const a := 12u", "const a := 12u;");
    test_reconstruction("const a := 12u", "const a := 12u;");
    test_reconstruction("const a := -12", "const a := -12;");
    test_reconstruction("const a := 1.236", "const a := 1.236;");
    test_reconstruction("const a := 1.236e2", "const a := 1.236e2;");
}

TEST_CASE("Short statements") {
    SECTION("Jump statements") {
        test_reconstruction("return", "return;");
        test_reconstruction("return nil", "return nil;");
        test_reconstruction("break", "break;");
        test_reconstruction("break -5.48e3", "break -5.48e3;");
        test_reconstruction("continue", "continue;");
    }

    SECTION("Import statements") {
        test_reconstruction("import std", "import std;");
        test_reconstruction(R"(import "array" as array)", R"(import "array" as array;)");
    }

    SECTION("Discard statements") {
        test_reconstruction("_ = 2", "_ = 2;");
        test_reconstruction("_ = [_]{}", "_ = [_]{ };");
    }

    SECTION("Impl statements") {
        test_reconstruction("impl Obj { const a := 1; }", "impl Obj { const a := 1; };");
    }
}

TEST_CASE("Conditionals") {
    SECTION("If expressions") {
        test_reconstruction("const a := if (x < 20) 2.345 else -2.3456e23",
                            "const a := if (x < 20) 2.345 else -2.3456e23;");
        test_reconstruction("return if (x + 2 == 30) { struct { a: int, b: int, } } else if (x < "
                            "2) { 4 } else { 4 }",
                            "return if (x + 2 == 30) { struct { a: int, b: int, } } else if (x < "
                            "2) { 4 } else { 4 };");
    }

    SECTION("Match expressions") {
        test_reconstruction("match Out { 1 => return 90u;, 2 => return 0b1011u, };",
                            "match Out { 1 => return 90u;, 2 => return 0b1011u;, }");
        test_reconstruction("match Out { 1 => { return first }, 2 => { return second }, }",
                            "match Out { 1 => { return first; }, 2 => { return second; }, }");
        test_reconstruction(
            "match Out { 1 => { return first }, 2 => { return second }, } else third",
            "match Out { 1 => { return first; }, 2 => { return second; }, } else third");
    }
}

TEST_CASE("Loops") {
    SECTION("For loops") {
        test_reconstruction("for (1) {1}", "for (1) { 1 }");
        test_reconstruction("for (1) : (name) {1}", "for (1) : (name) { 1 }");
        test_reconstruction("for (1, 2) : (name, _) {1}", "for (1, 2) : (name, _) { 1 }");
        test_reconstruction("for (1, 2) : (name, word) {1} else {1}",
                            "for (1, 2) : (name, word) { 1 } else { 1 }");
        test_reconstruction("for (1, 2) : (name, word) {1} else 1",
                            "for (1, 2) : (name, word) { 1 } else 1");
        test_reconstruction("for (1, 2, 3) : (name, ref hey, word) {1}",
                            "for (1, 2, 3) : (name, ref hey, word) { 1 }");
    }

    SECTION("While loops") {
        test_reconstruction("while (1) {1}", "while (1) { 1 }");
        test_reconstruction("while (i <= 20) : (i += 2) { i -= 1; }",
                            "while (i <= 20) : (i += 2) { i -= 1 }");
        test_reconstruction("while (c != '3') : (c *= 2) {1} else {1}",
                            "while (c != '3') : (c *= 2) { 1 } else { 1 }");
        test_reconstruction("while (1) : (1) {1} else 1u", "while (1) : (1) { 1 } else 1u");
    }

    SECTION("Do-while loops") {
        test_reconstruction(R"(do { print("Hello, World!") } while (true))",
                            R"(do { print("Hello, World!") } while (true))");
    }
}

TEST_CASE("Complex expressions") {
    SECTION("Function expressions") {
        test_reconstruction("fn(x: int, y: int, z: int): int {}",
                            "fn(x: int, y: int, z: int): int {  }");
        test_reconstruction("fn<T>(x: int, y: int, z: int = 2): int { if (x + y == z) return z; }",
                            "fn<T>(x: int, y: int, z: int = 2): int { if (x + y == z) return z; }");
        test_reconstruction("fn<T>(x: int, ref y: int, z: int = 2): Result<T, E> { if (x + y == z) "
                            "return Ok(3) with <int>; }",
                            "fn<T>(x: int, ref y: int, z: int = 2): Result<T, E> { if (x + y == z) "
                            "return Ok(3) with <int>; }");
    }

    SECTION("Call expressions") {
        test_reconstruction("add(1, 2)", "add(1, 2)");
        test_reconstruction("add(1, 2, ref result)", "add(1, 2, ref result)");
        test_reconstruction("add(0xFF, 2.0, ref result) with <uint, float>",
                            "add(0xFF, 2.0, ref result) with <uint, float>");
    }

    SECTION("Struct expressions") {
        test_reconstruction("struct { a: int, b: uint, c: ?Woah, d: int = 1, }",
                            "struct { a: int, b: uint, c: ?Woah, d: int = 1, }");
    }

    SECTION("Narrowed expressions") {
        test_reconstruction("Outer::inner", "Outer::inner");
        test_reconstruction("Outer::inner(45)", "Outer::inner(45)");
        test_reconstruction("Outer::inner(45, ref something) with <Other>",
                            "Outer::inner(45, ref something) with <Other>");
        test_reconstruction("Outer::inner = 45", "Outer::inner = 45");
    }
}

TEST_CASE("String literals") {
    SECTION("Standard strings") {
        test_reconstruction(R"(const a := "Hello, World!")", R"(const a := "Hello, World!";)");
        test_reconstruction(R"(const a: string = "Hey!")", R"(const a: string = "Hey!";)");
    }

    SECTION("Multiline strings") {
        test_reconstruction("var str := \\\\Multiline stringing\n\\\\\n",
                            "var str := \\\\Multiline stringing\n\\\\\n;");
        test_reconstruction("var str := \\\\Multiline stringing\n\\\\Continuation\n",
                            "var str := \\\\Multiline stringing\n\\\\Continuation\n;");
    }
}

TEST_CASE("Assignment") {
    SECTION("Simple assignment") {
        test_reconstruction("something = 2", "something = 2");
        test_reconstruction("something = 2 * 23 / 2", "something = 2 * 23 / 2");
    }

    SECTION("Compound assignment") {
        test_reconstruction("something += 2 + 0o237 & 6", "something += 2 + 0o237 & 6");
        test_reconstruction("something -= 2 + 0o237 & 6", "something -= 2 + 0o237 & 6");
        test_reconstruction("something *= 2 + 0o237 & 6", "something *= 2 + 0o237 & 6");
        test_reconstruction("something /= 2 + 0o237 & 6", "something /= 2 + 0o237 & 6");
        test_reconstruction("something %= 2 + 0o237 & 6", "something %= 2 + 0o237 & 6");
        test_reconstruction("something &= 2 + 0o237 & 6", "something &= 2 + 0o237 & 6");
        test_reconstruction("something |= 2 + 0o237 & 6", "something |= 2 + 0o237 & 6");
        test_reconstruction("something <<= 2 + 0o237 & 6", "something <<= 2 + 0o237 & 6");
        test_reconstruction("something >>= 2 + 0o237 & 6", "something >>= 2 + 0o237 & 6");
        test_reconstruction("something ~= 2 + 0o237 & 6", "something ~= 2 + 0o237 & 6");
        test_reconstruction("something ^= 2 + 0o237 & 6", "something ^= 2 + 0o237 & 6");
    }
}
