#include <catch_amalgamated.hpp>

#include <string>

#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

TEST_CASE("Illegal characters") {
    const std::string input{"月😭🎶"};
    Lexer             l{input};

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

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Symbols Only") {
        const std::string input{"=+(){}[],;: !-/*<>_"};
        Lexer             l{input};

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

        for (const auto& [t, s] : expecteds) {
            const auto token = l.advance();
            REQUIRE(t == token.type);
            REQUIRE(token.slice == s);
        }
    }

    SECTION("Basic Language Snippet") {
        const std::string input = "const five = 5;\n"
                                  "var ten = 10;\n\n"
                                  "var add = fn(x, y) {\n"
                                  "   x + y;\n"
                                  "};\n\n"
                                  "var result = add(five, ten);\n"
                                  "var four_and_some = 4.2;";

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
        Lexer      l{input};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s]           = expecteds[i];
            const auto token             = l.advance();
            const auto accumulated_token = accumulated_tokens[i];

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(token.slice == s);
            REQUIRE(accumulated_token.slice == s);
        }
    }
}
