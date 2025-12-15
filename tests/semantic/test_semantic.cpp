#include "catch_amalgamated.hpp"

#include "semantic_helpers.hpp"

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
        const char* inputs[] = {
            "const v := 3;",
            "const v := 3u;",
            "const v := \"3\";",
            "var v := '3';",
            "const v := true;",
            "var v := 5.234;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Correct explicit") {
        const char* inputs[] = {
            "const v: int = 3;",
            "const v: uint = 3u;",
            "const v: string = \"3\";",
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
        const char* inputs[] = {
            "const v: uint = 3;",
            "const v: int = 3u;",
            "const v: byte = \"3\";",
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
            const char* inputs[] = {
                "var v := 3; v = 5",
                "var v := 3u; v = 6u",
                "var v := \"3\"; v = \"woah\"",
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
            const char* inputs[] = {
                "var v: int = 3; v += 7",
                "var v: uint = 3u; v += 9u",
                "var v: ?uint = 3u; v += nil",
            };

            for (const auto& i : inputs) {
                test_analyze(i);
            }
        }

        SECTION("Incorrect assignments") {
            const char* input = "var v: int = 3; v += nil";
            test_analyze(input, {"TYPE_MISMATCH [Ln 1, Col 17]"});
        }
    }

    SECTION("Nil assignments") {
        SECTION("Correct assignment") {
            const char* inputs[] = {
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
