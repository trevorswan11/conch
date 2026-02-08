#include <catch_amalgamated.hpp>

#include <utility>

#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

using namespace conch;

TEST_CASE("Illegal characters") {
    const auto input{"月😭🎶"};
    Lexer      l{input};

    const auto tokens = l.consume();
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (i == tokens.size() - 1) {
            REQUIRE(token.type == TokenType::END);
            break;
        }
        REQUIRE(token.type == TokenType::ILLEGAL);
    }
}

TEST_CASE("Lexer over-consumption") {
    const auto input{"lexer"};
    Lexer      l{input};

    l.consume();
    for (size_t i = 0; i < 100; ++i) {
        REQUIRE(l.advance().type == TokenType::END);
    }
}

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Symbols Only") {
        const auto input{"=+(){}[],;: !-/*<>_"};
        Lexer      l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::ASSIGN, "="},   std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::LPAREN, "("},   std::pair{TokenType::RPAREN, ")"},
            std::pair{TokenType::LBRACE, "{"},   std::pair{TokenType::RBRACE, "}"},
            std::pair{TokenType::LBRACKET, "["}, std::pair{TokenType::RBRACKET, "]"},
            std::pair{TokenType::COMMA, ","},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::COLON, ":"},    std::pair{TokenType::BANG, "!"},
            std::pair{TokenType::MINUS, "-"},    std::pair{TokenType::SLASH, "/"},
            std::pair{TokenType::STAR, "*"},     std::pair{TokenType::LT, "<"},
            std::pair{TokenType::GT, ">"},       std::pair{TokenType::UNDERSCORE, "_"},
            std::pair{TokenType::END, ""},
        };

        for (const auto& [expected_tok, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tok == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Basic Language Snippet") {
        const auto input{"const five = 5;\n"
                         "var ten = 10;\n\n"
                         "var add = fn(x, y) {\n"
                         "   x + y;\n"
                         "};\n\n"
                         "var result = add(five, ten);\n"
                         "var four_and_some = 4.2;"};
        Lexer      l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::CONST, "const"}, std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::INT_10, "5"},
            std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "ten"},   std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::INT_10, "10"},   std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FUNCTION, "fn"},
            std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "x"},
            std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "y"},
            std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::LBRACE, "{"},
            std::pair{TokenType::IDENT, "x"},     std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::IDENT, "y"},     std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::RBRACE, "}"},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "result"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "ten"},
            std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "four_and_some"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FLOAT, "4.2"},
            std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::END, ""},
        };

        Lexer      l_accumulator{input};
        const auto accumulated_tokens = l_accumulator.consume();
        l_accumulator.reset(input);
        const auto reset_acc = l_accumulator.consume();

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [expected_tok, expected_slice] = expecteds[i];
            const auto token                           = l.advance();
            const auto accumulated_token               = accumulated_tokens[i];
            const auto reset                           = reset_acc[i];

            REQUIRE(expected_tok == token.type);
            REQUIRE(expected_tok == accumulated_token.type);
            REQUIRE(expected_slice == token.slice);
            REQUIRE(expected_slice == accumulated_token.slice);
            REQUIRE(accumulated_token == reset);
        }
    }
}

TEST_CASE("Base-10 ints and floats") {
    SECTION("Correct numbers") {
        const auto input{"0 123 3.14 42.0 1e20 1.e-3 2.3901E4 1e."};
        Lexer      l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::INT_10, "0"},
            std::pair{TokenType::INT_10, "123"},
            std::pair{TokenType::FLOAT, "3.14"},
            std::pair{TokenType::FLOAT, "42.0"},
            std::pair{TokenType::FLOAT, "1e20"},
            std::pair{TokenType::FLOAT, "1.e-3"},
            std::pair{TokenType::FLOAT, "2.3901E4"},
            std::pair{TokenType::INT_10, "1"},
            std::pair{TokenType::IDENT, "e"},
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::END, ""},
        };

        for (const auto& [expected_token, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_token == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Illegal Floats") {
        const auto input{".0 1..2 3.4.5 3.4u"};
        Lexer      l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::INT_10, "0"},
            std::pair{TokenType::INT_10, "1"},
            std::pair{TokenType::DOT_DOT, ".."},
            std::pair{TokenType::INT_10, "2"},
            std::pair{TokenType::FLOAT, "3.4"},
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::INT_10, "5"},
            std::pair{TokenType::FLOAT, "3.4"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::END, ""},
        };

        for (const auto& [expected_token, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_token == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }
}

TEST_CASE("Integer base variants") {
    SECTION("Signed integer variants") {
        const auto input{"0b1010 0o17 0O17 42 0x2A 0X2A 0b 0x 0o"};
        Lexer      l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::INT_2, "0b1010"},
            std::pair{TokenType::INT_8, "0o17"},
            std::pair{TokenType::INT_8, "0O17"},
            std::pair{TokenType::INT_10, "42"},
            std::pair{TokenType::INT_16, "0x2A"},
            std::pair{TokenType::INT_16, "0X2A"},
            std::pair{TokenType::ILLEGAL, "0b"},
            std::pair{TokenType::ILLEGAL, "0x"},
            std::pair{TokenType::ILLEGAL, "0o"},
            std::pair{TokenType::END, ""},
        };

        for (const auto& [expected_token, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_token == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Unsigned integer variants") {
        const auto input{
            "0b1010u 0b1010uz 0o17u 0o17uz 0O17u 42u 42UZ 0x2AU 0X2Au 123ufoo 0bu 0xu 0ou"};
        Lexer l{input};

        const auto expecteds = std::array{
            std::pair{TokenType::UINT_2, "0b1010u"},
            std::pair{TokenType::UZINT_2, "0b1010uz"},
            std::pair{TokenType::UINT_8, "0o17u"},
            std::pair{TokenType::UZINT_8, "0o17uz"},
            std::pair{TokenType::UINT_8, "0O17u"},
            std::pair{TokenType::UINT_10, "42u"},
            std::pair{TokenType::UZINT_10, "42UZ"},
            std::pair{TokenType::UINT_16, "0x2AU"},
            std::pair{TokenType::UINT_16, "0X2Au"},
            std::pair{TokenType::UINT_10, "123u"},
            std::pair{TokenType::IDENT, "foo"},
            std::pair{TokenType::ILLEGAL, "0b"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::ILLEGAL, "0x"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::ILLEGAL, "0o"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::END, ""},
        };

        for (const auto& [expected_token, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_token == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }
}

TEST_CASE("Iterator with other keywords") {
    const auto input{"private extern export packed"};
    Lexer      l{input};

    const auto expecteds = std::array{
        std::pair{TokenType::PRIVATE, "private"},
        std::pair{TokenType::EXTERN, "extern"},
        std::pair{TokenType::EXPORT, "export"},
        std::pair{TokenType::PACKED, "packed"},
    };

    size_t i = 0;
    for (const auto& token : l) {
        REQUIRE(expecteds[i].first == token.type);
        REQUIRE(expecteds[i].second == token.slice);
        i += 1;
    }
}

TEST_CASE("Comments") {
    const auto input{"const five = 5;\n"
                     "var ten_10 = 10;\n\n"
                     "// Comment on a new line\n"
                     "var add = fn(x, y) {\n"
                     "   x + y;\n"
                     "};\n\n"
                     "var result = add(five, ten); // This is an end of line comment\n"
                     "var four_and_some = 4.2;\n"
                     "work;"};
    Lexer      l{input};

    const auto expecteds = std::array{
        std::pair{TokenType::CONST, "const"},
        std::pair{TokenType::IDENT, "five"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::INT_10, "5"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "ten_10"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::INT_10, "10"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::COMMENT, " Comment on a new line"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "add"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::FUNCTION, "fn"},
        std::pair{TokenType::LPAREN, "("},
        std::pair{TokenType::IDENT, "x"},
        std::pair{TokenType::COMMA, ","},
        std::pair{TokenType::IDENT, "y"},
        std::pair{TokenType::RPAREN, ")"},
        std::pair{TokenType::LBRACE, "{"},
        std::pair{TokenType::IDENT, "x"},
        std::pair{TokenType::PLUS, "+"},
        std::pair{TokenType::IDENT, "y"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::RBRACE, "}"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "result"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::IDENT, "add"},
        std::pair{TokenType::LPAREN, "("},
        std::pair{TokenType::IDENT, "five"},
        std::pair{TokenType::COMMA, ","},
        std::pair{TokenType::IDENT, "ten"},
        std::pair{TokenType::RPAREN, ")"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::COMMENT, " This is an end of line comment"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "four_and_some"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::FLOAT, "4.2"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::IDENT, "work"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::END, ""},
    };

    for (const auto& [expected_token, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_token == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("Character literals") {
    const auto input{"if'e' else'\\'\nreturn'\\r' break'\\n'\n"
                     "continue'\\0' for'\\'' while'\\\\' const''\n"
                     "var'asd'"};
    Lexer      l{input};

    const auto expecteds = std::array{
        std::pair{TokenType::IF, "if"},
        std::pair{TokenType::BYTE, "'e'"},
        std::pair{TokenType::ELSE, "else"},
        std::pair{TokenType::ILLEGAL, "'\\'"},
        std::pair{TokenType::RETURN, "return"},
        std::pair{TokenType::BYTE, "'\\r'"},
        std::pair{TokenType::BREAK, "break"},
        std::pair{TokenType::BYTE, "'\\n'"},
        std::pair{TokenType::CONTINUE, "continue"},
        std::pair{TokenType::BYTE, "'\\0'"},
        std::pair{TokenType::FOR, "for"},
        std::pair{TokenType::BYTE, "'\\''"},
        std::pair{TokenType::WHILE, "while"},
        std::pair{TokenType::BYTE, "'\\\\'"},
        std::pair{TokenType::CONST, "const"},
        std::pair{TokenType::ILLEGAL, "'"},
        std::pair{TokenType::ILLEGAL, "'"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::ILLEGAL, "'asd'"},
        std::pair{TokenType::END, ""},
    };

    for (const auto& [expected_token, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_token == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("String literals") {
    const auto input{
        R"("This is a string";const five = "Hello, World!";var ten = "Hello\n, World!\0";var one := "Hello, World!;)"};
    Lexer l{input};

    const auto expecteds = std::array{
        std::pair{TokenType::STRING, R"("This is a string")"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::CONST, "const"},
        std::pair{TokenType::IDENT, "five"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::STRING, R"("Hello, World!")"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "ten"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::STRING, R"("Hello\n, World!\0")"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "one"},
        std::pair{TokenType::WALRUS, ":="},
        std::pair{TokenType::ILLEGAL, R"("Hello, World!;)"},
        std::pair{TokenType::END, ""},
    };

    for (const auto& [expected_token, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_token == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("Multiline string literals") {
    const auto input{"const five = \\\\Multiline stringing\n"
                     ";\n"
                     "var ten = \\\\Multiline stringing\n"
                     "\\\\Continuation\n"
                     ";\n"
                     "const one = \\\\Nesting \" \' \\ [] const var\n"
                     "\\\\\n"
                     ";\n"};
    Lexer      l{input};

    const auto expecteds = std::array{
        std::pair{TokenType::CONST, "const"},
        std::pair{TokenType::IDENT, "five"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::MULTILINE_STRING, "Multiline stringing"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::VAR, "var"},
        std::pair{TokenType::IDENT, "ten"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::MULTILINE_STRING, "Multiline stringing\n\\\\Continuation"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::CONST, "const"},
        std::pair{TokenType::IDENT, "one"},
        std::pair{TokenType::ASSIGN, "="},
        std::pair{TokenType::MULTILINE_STRING, "Nesting \" \' \\ [] const var\n\\\\"},
        std::pair{TokenType::SEMICOLON, ";"},
        std::pair{TokenType::END, ""},
    };

    for (const auto& [expected_token, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_token == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}
