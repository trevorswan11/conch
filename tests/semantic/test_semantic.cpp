#include "catch_amalgamated.hpp"

#include <optional>
#include <string>
#include <vector>

#include "fixtures.hpp"

extern "C" {
#include "util/arena.h"
}

static void test_analyze(const char*                     input,
                         const std::vector<std::string>& expected_errors,
                         bool                            print_anyways) {
    SemanticFixture sf_std{input, &std_allocator};
    sf_std.check_errors(expected_errors, print_anyways);

    // Rerun tests with arena allocator
    Allocator arena;
    REQUIRE(STATUS_OK(arena_init(&arena, ARENA_DEFAULT_SIZE, &std_allocator)));
    const Fixture<Allocator> af(arena, arena_deinit);

    const std::optional<ArenaResetMode> reset_modes[] = {
        std::nullopt,
        ArenaResetMode::DEFAULT,
        ArenaResetMode::RETAIN_CAPACITY,
        ArenaResetMode::ZERO_RETAIN_CAPACITY,
        ArenaResetMode::FULL_RESET,
        std::nullopt,
    };

    // Go through the reset modes to test each arena configuration
    for (const auto& mode : reset_modes) {
        // This has to be scoped since the reset can leave dangling pointers
        {
            SemanticFixture sf_arena{input, &arena};
            sf_arena.check_errors(expected_errors, print_anyways);
        }

        if (mode.has_value()) { arena_reset(&arena, mode.value()); }
    }
}

static void test_analyze(const char* input, const std::vector<std::string>& expected_errors = {}) {
    const bool empty = expected_errors.empty();
    test_analyze(input, expected_errors, empty);
}

TEST_CASE("Basic identifiers") {
    SECTION("Identifier declaration") {
        const char* input = "const a := 2; const b := a;";
        test_analyze(input);
    }

    SECTION("Undeclared identifier") {
        const char* input = "const a := b;";
        test_analyze(input, {"UNDECLARED_IDENTIFIER [Ln 1, Col 12]"});
    }

    SECTION("Duplicate identifier") {
        const char* input = "const a := 2; const a := 3;";
        test_analyze(input, {"REDEFINITION_OF_IDENTIFIER [Ln 1, Col 21]"});
    }
}

TEST_CASE("Primitive declarations") {
    SECTION("Implicit") {
        const char* const inputs[] = {
            "const v := 3;",
            "const v := 3u;",
            R"(const v := "3";)",
            "var v := '3';",
            "const v := true;",
            "var v := 5.234;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Correct explicit") {
        const char* const inputs[] = {
            "const v: int = 3;",
            "const v: uint = 3u;",
            "const v: size = 5uz;",
            "const v: ?size = nil;",
            R"(const v: string = "3";)",
            "const v: byte = '3';",
            "const v: bool = true;",
            "const v: float = 5.234;",
            "const v: ?float = 5.234;",
            "const v: ?float = nil;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Incorrect explicit") {
        const char* const inputs[] = {
            "const v: uint = 3;",
            "const v: int = 3u;",
            "const v: ?size = 3u;",
            "const v: size = 3u;",
            R"(const v: byte = "3";)",
            "const v: string = '3';",
            "const v: float = true;",
            "const v: bool = 5.234;",
            "const v: float = nil;",
        };

        for (const auto& i : inputs) {
            test_analyze(i, {"TYPE_MISMATCH [Ln 1, Col 1]"});
        }
    }
}

TEST_CASE("Assignment expressions") {
    SECTION("Basic assignments") {
        SECTION("Correct assignment") {
            const char* const inputs[] = {
                "var v := 3; v = 5",
                "var v := 3u; v = 6u",
                "var v := 3uz; v = 6uz",
                R"(var v := "3"; v = "woah")",
                "var v := '3'; v = '\\0'",
                "var v := true; v = false",
                "var v := 5.234; v = 2.34e30",
            };

            for (const auto& i : inputs) {
                test_analyze(i);
            }
        }

        SECTION("Undeclared identifiers") {
            const char* input = "var a := 4; a = b";
            test_analyze(input, {"UNDECLARED_IDENTIFIER [Ln 1, Col 17]"});
        }

        SECTION("Constant assignment") {
            const char* input = "const a := 4; a = 5";
            test_analyze(input, {"ASSIGNMENT_TO_CONSTANT [Ln 1, Col 15]"});
        }
    }

    SECTION("Compound assignments") {
        SECTION("Correct assignment") {
            // Adding nil to a nullable value will be a runtime check
            const char* const inputs[] = {
                "var v: int = 3; v += 7",
                "var v: uint = 3u; v += 9u",
                "var v: uint = 3u; v *= 9u",
                "var v: uint = 3u; v &= 9u",
                "var v: uint = 3u; v |= 9u",
                "var v: uint = 3u; v ^= 9u",
                "var v: uint = 3u; v <<= 9u",
                "var v: uint = 3u; v >>= 9u",
                "var v: uint = 3u; v %= 9u",
                "var v: uint = 3u; v -= 9u",
                "var v: uint = 3u; v /= 9u",
                "var v: uint = 3u; v ~= 9u",
                R"(var v: string = "H"; v += "i")",
                R"(var v: string = "H"; v *= "i")",
            };

            for (const auto& i : inputs) {
                test_analyze(i);
            }
        }

        SECTION("Incorrect assignments") {
            test_analyze("var v: int = 3; v += nil", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 17]"});
            test_analyze("var v: int = 3; v += 3u", {"TYPE_MISMATCH [Ln 1, Col 17]"});
            test_analyze("var v: int = 3; v ~= 3u", {"TYPE_MISMATCH [Ln 1, Col 17]"});
            test_analyze("var v: ?int = 3; v += nil", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 18]"});
            test_analyze("type a = enum { ONE, }; var v := 2; v += a",
                         {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 37]"});
            test_analyze("type a = enum { ONE, }; var v: a = a::ONE; v ~= 3",
                         {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 44]"});
            test_analyze("var v: int; v += 2", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 13]"});
        }
    }

    SECTION("Nil assignments") {
        SECTION("Correct assignment") {
            const char* const inputs[] = {
                "var v: ?int = 3; v = nil",
                "var v: ?uint = 3u; v = nil",
            };

            for (const auto& i : inputs) {
                test_analyze(i);
            }
        }

        SECTION("Incorrect assignments") {
            const char* input = "var v: int = 3; v = nil";
            test_analyze(input, {"TYPE_MISMATCH [Ln 1, Col 17]"});
        }
    }
}

TEST_CASE("Discard and ignore expressions") {
    SECTION("Discard assignments") {
        SECTION("Valid discard assignment") {
            const char* input = "_ = 4";
            test_analyze(input);
        }

        SECTION("Invalid discard assignment") {
            const char* input = "_ = a";
            test_analyze(input, {"UNDECLARED_IDENTIFIER [Ln 1, Col 5]"});
        }
    }
}

TEST_CASE("Block statements") {
    SECTION("Correct top-level blocks") {
        const char* input = "var v := 3; { v = 5 }";
        test_analyze(input);
    }

    SECTION("Incorrect top-level blocks") {
        const char* input = "var v := 3; { a = 5 }";
        test_analyze(input, {"UNDECLARED_IDENTIFIER [Ln 1, Col 15]"});
    }
}

TEST_CASE("Type aliases") {
    SECTION("Primitive aliases") {
        const char* const inputs[] = {
            "type custom = int; const v: custom = 3;",
            "type custom = uint; const v: custom = 3u;",
            "type custom = size; const v: custom = 3uz;",
            R"(type custom = string; const v: custom = "3";)",
            "type custom = byte; const v: custom = '3';",
            "type custom = bool; const v: custom = true;",
            "type custom = float; const v: custom = 5.234;",
            "type custom = ?float; var v: custom = 5.234; v = nil",
            "type custom = ?float; var v: custom = nil; v = 2.346",
            "type custom = ?uint; var v: custom; v = 2u; v = nil",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Undeclared primitive alias") {
        test_analyze("const v: custom = 3", {"UNDECLARED_IDENTIFIER [Ln 1, Col 7]"});
    }

    SECTION("Double nullables") {
        test_analyze("type custom = ?int; type c = ?custom", {"DOUBLE_NULLABLE [Ln 1, Col 28]"});
        test_analyze("type custom = ?int; var c: ?custom", {"DOUBLE_NULLABLE [Ln 1, Col 25]"});
        test_analyze("type c1 = ?int; type c2 = c1; type c3 = ?c2",
                     {"DOUBLE_NULLABLE [Ln 1, Col 39]"});
    }
}

TEST_CASE("Enum types") {
    SECTION("Correct standard declarations") {
        test_analyze("type g = enum { ONE, }; var a: g; var b: g; a = b");
        test_analyze("type g = enum { ONE, TWO = 2, THREE, }; var a: g; var b: g; a = b");
        test_analyze(
            "const val := 3; type g = enum { ONE, TWO = val, THREE, }; var a: g; var b: g; a = b");
    }

    SECTION("Assignment flavors") {
        const char* const inputs[] = {
            "type A = enum { a, }; const b: A = A::a",
            "type A = ?enum { a, }; var b: A = A::a; b = nil",
            "type A = enum { RED, BLUE, GREEN, }; var b: A = A::RED; b = A::BLUE; b = A::GREEN",
            "type A = ?enum { a, }; const b: A = A::a; var c: A = nil; c = b",
            "type A = enum { a, }; type B = ?A; const b: A = A::a; var c: B = nil; c = b",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Error cases") {
        test_analyze("var val := 3; type g = enum { ONE, TWO = val, THREE, };",
                     {"NON_CONST_ENUM_VARIANT [Ln 1, Col 24]"});
        test_analyze("const val: ?int = 3; type g = enum { ONE, TWO = val, THREE, };",
                     {"NULLABLE_ENUM_VARIANT [Ln 1, Col 31]"});
        test_analyze("const val: uint = 3u; type g = enum { ONE, TWO = val, THREE, };",
                     {"NON_SIGNED_ENUM_VARIANT [Ln 1, Col 32]"});
        test_analyze("type T = int; type g = enum { ONE, TWO = T, THREE, };",
                     {"NON_VALUED_ENUM_VARIANT [Ln 1, Col 24]"});
        test_analyze("type A = enum { a, }; type B = ?A; type C = ?B",
                     {"DOUBLE_NULLABLE [Ln 1, Col 43]"});
        test_analyze("type A = enum { a, }; const b := a",
                     {"UNDECLARED_IDENTIFIER [Ln 1, Col 34]"});
        test_analyze("var a: enum { b, }", {"ANONYMOUS_ENUM [Ln 1, Col 8]"});
        test_analyze("enum { ONE, TWO, THREE, }", {"ANONYMOUS_ENUM [Ln 1, Col 1]"});
        test_analyze("type a = enum {ONE,}; a::TWO", {"UNKNOWN_ENUM_VARIANT [Ln 1, Col 23]"});
    }
}

TEST_CASE("Prefix operators") {
    SECTION("Bang operator") {
        SECTION("Correct types") {
            test_analyze("const a: bool = !false; const b: bool = !nil;");
            test_analyze("const a: bool = !2u; const b: bool = !2u; const c: bool = !3.23");
            test_analyze(R"(const a: bool = !"Value"; const b: bool = !'v';)");
            test_analyze("type g = ?enum { ONE, }; const d := !g");
        }

        SECTION("Incorrect types") {
            test_analyze("type g = enum { ONE, }; const d := !g",
                         {"ILLEGAL_PREFIX_OPERAND [Ln 1, Col 37]"});
        }
    }

    SECTION("Not and minus operators") {
        SECTION("Correct types") {
            test_analyze("const a: int = ~0b011011; const b: uint = ~0b011011u;");
            test_analyze("const a: int = -0b011011; const b: uint = -0b011011u;");
            test_analyze("const a: float = -2.34; const b: float = -3.14e-100;");
            test_analyze("type T = ?float; var v: T = nil; v = 2.3; v = -4.5e10");
            test_analyze("var v := 2u; v = ~v");
            test_analyze("var v: uint = 2u; v = ~v");
            test_analyze("type T = uint; var v: T = 2u; v = ~v");
        }

        SECTION("Incorrect types") {
            test_analyze("var v: uint = 2u; v = ~2", {"TYPE_MISMATCH [Ln 1, Col 19]"});
            test_analyze("var v: uint = 2u; v = -2", {"TYPE_MISMATCH [Ln 1, Col 19]"});
            test_analyze("var v: ?uint = 2u; v = ~v", {"ILLEGAL_PREFIX_OPERAND [Ln 1, Col 25]"});
            test_analyze("type E = enum { A, }; var v := !E::A",
                         {"ILLEGAL_PREFIX_OPERAND [Ln 1, Col 33]"});
        }
    }
}

TEST_CASE("Infix operators") {
    SECTION("Addition and multiplication") {
        test_analyze(R"(const a: string = "Result: "; const b: int = 5; const c: string = a + b)");
        test_analyze(R"(const a: string = "Result: "; const b: int = 5; const c: string = a * b)");
        test_analyze("const a: int = 3; const b: int = 5; const c: int = a + b");
        test_analyze("const a: uint = 3u; const b: uint = 5u; const c := a + b");
        test_analyze("const a := 3.3; const b := 5.5; const c: float = a + b");
        test_analyze("const a := 3u; const b: uint = 5u; const c := a * b");
    }

    SECTION("Modulo, bitwise, and logic") {
        test_analyze("3 % 4");
        test_analyze("3u % 4u");
        test_analyze("3u % -4u");
        test_analyze("3 & 4");
        test_analyze("3u | 4u");
        test_analyze("3u ^ -4u");
        test_analyze("3 << 4");
        test_analyze("3u >> 4u");
        test_analyze("3u < 4u");
        test_analyze("'3' < '4'");
        test_analyze("3u <= -4u");
        test_analyze("3u > 4u");
        test_analyze("'3' > '4'");
        test_analyze("'3' > '4'");
        test_analyze("3 >= 4");
        test_analyze("'3' >= '4'");
        test_analyze("3u == nil");
        test_analyze(R"(nil != "Hello")");
        test_analyze("nil == nil");
        test_analyze("3 != 4");
        test_analyze("const a: ?int = 3; 6 == a");
        test_analyze("3 is 4");
        test_analyze("3 is 'd'");
        test_analyze(R"(3 is "Hello")");
        test_analyze("true and true");
        test_analyze("nil and false");
        test_analyze("const a: ?int = 3; a or nil");
        test_analyze("const a: ?int = 3; true and a");
        test_analyze("true or true");
        test_analyze("true or nil");
    }

    SECTION("Fallback arithmetic") {
        test_analyze("3 - 4");
        test_analyze("3 / 4");
        test_analyze("3 / 4");
        test_analyze("3 ** 4");
        test_analyze("var v: uint = 3u ** 4u; v = 45u");
    }

    SECTION("Range and in expressions") {
        test_analyze("3..4");
        test_analyze("3u..4u");
        test_analyze("3uz..4uz");
        test_analyze("3uz..=4uz");
        test_analyze("const a: int = 5; const b: int = 34; a..=b");
        test_analyze("3 in 5..6");
    }

    SECTION("Orelse expressions") {
        test_analyze("const a: ?int = 3; var b: int = a orelse 6");
        test_analyze("const a: ?uint = 3u; var b: uint = a orelse 6u");
    }

    SECTION("Error cases") {
        test_analyze("type a = int; type b = uint; a + b",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 30]"});
        test_analyze("type a = uint; 4 + a", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 16]"});
        test_analyze(R"(const a: string = "Hello"; const b: ?int = 3; a + b)",
                     {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 47]"});
        test_analyze(R"(const a: ?string = "Hello"; const b: int = 3; a + b)",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 47]"});
        test_analyze("const a: byte = '3'; const b: byte = '5'; const c := a + b",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 54]"});
        test_analyze(R"(const a: int = 5; const b: string = "Result: "; const c: string = a + b)",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 67]"});
        test_analyze(R"(const a: int = 5; const b: string = "Result: "; const c: string = a * b)",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 67]"});
        test_analyze("const a := 3u; const b: uint = 5u; const c: int = a * b",
                     {"TYPE_MISMATCH [Ln 1, Col 36]"});
        test_analyze("3.34 % 4", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3.34 ** 4", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("3 % 4.45", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3.23 % 4.45", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3.23 orelse 4.45", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3 == 4.45", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("nil orelse 4", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("const a: ?int = 3; const c: ?int = nil; var b: int = a orelse c",
                     {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 54]"});
        test_analyze("true or 4", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("5u or false", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("true and 4", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("7u and 4", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("type a = enum { ONE, }; const b: a = a::ONE; b == 4",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 46]"});
        test_analyze(R"("tre" < "vor")", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3 < 4.3", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("3.3..=4", {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3..=4.3", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 1]"});
        test_analyze("3..=4u", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("4 in 3u..=4u", {"TYPE_MISMATCH [Ln 1, Col 1]"});
        test_analyze("type a = enum { A, }; a::A in 3u..=4u",
                     {"ILLEGAL_LHS_INFIX_OPERAND [Ln 1, Col 23]"});
        test_analyze("3 in 4", {"ILLEGAL_RHS_INFIX_OPERAND [Ln 1, Col 1]"});
    }
}

TEST_CASE("Arrays") {
    SECTION("Inferred typed arrays") {
        SECTION("Definitions") {
            test_analyze("[4uz]{1, 3, 5, 6, }");
            test_analyze("[_]{1u, 3u, 5u, 6u, }");
            test_analyze(R"(const strs := [_]{"hello", "world", "!", }; strs[2uz])");
            test_analyze(R"([_]{[_]{"hello", "world", "!", }, [_]{"hello", "world", "!", }, })");
            test_analyze(R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, };)");
            test_analyze(R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, }[9uz];)");
            test_analyze(R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, }[0uz..1uz];)");
            test_analyze(R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, }[0uz..=1uz];)");
            test_analyze(
                R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, }[2uz] = [_]{"hello", "world", "?", };)");
            test_analyze("var a := 3..6; a[2uz] = 4");
            test_analyze(
                R"(var d := [_]{ [_]{ "hello", "world", "!", }, [_]{ "hello", "world", "!", }, };
                    var a := [_]{ d, d, d, d, }; a[0uz] = [_]{ [_]{ "hello", "world", "?", },
                    [_]{ "hello", "world", "?", }, }; )");
        }

        SECTION("Type mismatches") {
            test_analyze("[4uz]{1, 3u, 5, 6, }", {"ARRAY_ITEM_TYPE_MISMATCH [Ln 1, Col 1]"});
            test_analyze("[_]{[4uz]{1, 3, 5, 6, }, [3uz]{1, 3, 5, }, }",
                         {"ARRAY_ITEM_TYPE_MISMATCH [Ln 1, Col 1]"});
            test_analyze("[4uz]{1, 3, 5, 6, }[0..1]", {"TYPE_MISMATCH [Ln 1, Col 21]"});
            test_analyze("[4uz]{1, 3, 5, 6, }[0u..=1u]", {"TYPE_MISMATCH [Ln 1, Col 21]"});
            test_analyze("[4uz]{1, 3, 5, 6, }[3uz] = 4u", {"TYPE_MISMATCH [Ln 1, Col 20]"});
            test_analyze(
                R"(var a := [_]{"hello", "world", "!", }; [_]{a, a, a, a, }[2uz] = [_]{"hello", ",", "world", "?", };)",
                {"TYPE_MISMATCH [Ln 1, Col 57]"});
            test_analyze("type a = enum {ONE, }; [_]{a, }",
                         {"NON_VALUED_ARRAY_ITEM [Ln 1, Col 24]"});
            test_analyze("[_]{3u..5u, }", {"ILLEGAL_ARRAY_ITEM [Ln 1, Col 1]"});
        }
    }

    SECTION("Explicitly typed") {
        SECTION("Definitions") {
            test_analyze(
                R"(const d: [2uz, 3uz]string = [_]{ [_]{ "hello", "world", "!", },
                    [_]{ "hello", "world", "!", }, };)");
            test_analyze(
                R"(const d: [2uz, 3uz]string = [_]{ [_]{ "hello", "world", "!", },
                    [_]{ "hello", "world", "!", }, };
                    const a: [4uz, 2uz, 3uz]string = [_]{ d, d, d, d, };)");
        }

        SECTION("Type mismatch") {
            test_analyze(
                R"(const d: [3uz, 3uz]string = [_]{ [_]{ "hello", "world", "!", },
                    [_]{ "hello", "world", "!", }, };)",
                {"TYPE_MISMATCH [Ln 1, Col 1]"});
            test_analyze(
                R"(const d: [2uz, 2uz]string = [_]{ [_]{ "hello", "world", "!", },
                    [_]{ "hello", "world", "!", }, };)",
                {"TYPE_MISMATCH [Ln 1, Col 1]"});
        }
    }

    SECTION("Indexing") {
        SECTION("Range Expressions") {
            test_analyze("const a := 3..60; const b := 20uz; a[b]");
            test_analyze("const a := 3..6; const b: int = a[2uz]");
            test_analyze("const a := 3u..=6u; var b: ?uint = a[3uz]");
        }

        SECTION("Incorrect indexing") {
            test_analyze("3[4]", {"NON_ARRAY_INDEX_TARGET [Ln 1, Col 2]"});
            test_analyze("const a := 3..6; const b: int = a[2u]",
                         {"UNEXPECTED_ARRAY_INDEX_TYPE [Ln 1, Col 35]"});
            test_analyze("const a := 3..6; const b: int = a[2]",
                         {"UNEXPECTED_ARRAY_INDEX_TYPE [Ln 1, Col 35]"});
            test_analyze("const a := 3..6; a[2uz] = 4", {"ASSIGNMENT_TO_CONSTANT [Ln 1, Col 19]"});
        }
    }
}

TEST_CASE("Illegal namespaces") {
    test_analyze("1::ONE", {"ILLEGAL_OUTER_NAMESPACE [Ln 1, Col 1]"});
}

TEST_CASE("Type introspection") {
    SECTION("Enum introspection") {
        test_analyze(
            "type g = enum { ONE, }; type G = typeof g; var a: G = g::ONE; var b: g; a = b");
        test_analyze("type g = enum { ONE, }; type G = ?typeof g; var a: G = g::ONE; a = nil;");
        test_analyze("type g = enum { ONE, }; type G = ?typeof g; const a: G = G::ONE;");
    }
}
