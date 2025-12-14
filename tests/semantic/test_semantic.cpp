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
            "const v := '3';",
            "const v := true;",
            "const v := 5.234;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Correct explicit") {
        const const char* inputs[] = {
            "const v: int = 3;",
            "const v: uint = 3u;",
            "const v: string = \"3\";",
            "const v: byte = '3';",
            "const v: bool = true;",
            "const v: float = 5.234;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }

    SECTION("Incorrect explicit") {
        const const char* inputs[] = {
            "const v: uint = 3;",
            "const v: int = 3u;",
            "const v: byte = \"3\";",
            "const v: string = '3';",
            "const v: float = true;",
            "const v: bool = 5.234;",
        };

        for (const auto& i : inputs) {
            test_analyze(i, {"TYPE_MISMATCH [Ln 1, Col 1]"}, false);
        }
    }
}
