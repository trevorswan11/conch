#include <catch_amalgamated.hpp>

#include "lexer/lexer.hpp"

TEST_CASE("Illegal characters") {
    const char* input = "月😭🎶";

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

TEST_CASE("Basic next token and lexer consuming") {
    // SECTION("Symbols Only") {
    //     const char* input = "=+(){}[],;: !-/*<>_";

    //     const auto expecteds = std::array{
    //         std::pair{TokenType::ASSIGN, "="},   std::pair{TokenType::PLUS, "+"},
    //         std::pair{TokenType::LPAREN, "("},   std::pair{TokenType::RPAREN, ")"},
    //         std::pair{TokenType::LBRACE, "{"},   std::pair{TokenType::RBRACE, "}"},
    //         std::pair{TokenType::LBRACKET, "["}, std::pair{TokenType::RBRACKET, "]"},
    //         std::pair{TokenType::COMMA, ","},    std::pair{TokenType::SEMICOLON, ";"},
    //         std::pair{TokenType::COLON, ":"},    std::pair{TokenType::BANG, "!"},
    //         std::pair{TokenType::MINUS, "-"},    std::pair{TokenType::SLASH, "/"},
    //         std::pair{TokenType::STAR, "*"},     std::pair{TokenType::LT, "<"},
    //         std::pair{TokenType::GT, ">"},       std::pair{TokenType::UNDERSCORE, "_"},
    //         std::pair{TokenType::END, ""},
    //     };

    //     Lexer l;
    //     REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
    //     const Fixture<Lexer> lf{l, lexer_deinit};

    //     for (const auto& [t, s] : expecteds) {
    //         const auto token = lexer_next_token(&l);
    //         REQUIRE(t == token.type);
    //         REQUIRE(slice_equals_str_z(&token.slice, s));
    //     }
    // }

    // SECTION("Basic Language Snippet") {
    //     const char* input = "const five = 5;\n"
    //                         "var ten = 10;\n\n"
    //                         "var add = fn(x, y) {\n"
    //                         "   x + y;\n"
    //                         "};\n\n"
    //                         "var result = add(five, ten);\n"
    //                         "var four_and_some = 4.2;";

    //     const auto expecteds = std::array{
    //         std::pair{TokenType::CONST, "const"}, std::pair{TokenType::IDENT, "five"},
    //         std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::INT_10, "5"},
    //         std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::VAR, "var"},
    //         std::pair{TokenType::IDENT, "ten"},   std::pair{TokenType::ASSIGN, "="},
    //         std::pair{TokenType::INT_10, "10"},   std::pair{TokenType::SEMICOLON, ";"},
    //         std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "add"},
    //         std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FUNCTION, "fn"},
    //         std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "x"},
    //         std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "y"},
    //         std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::LBRACE, "{"},
    //         std::pair{TokenType::IDENT, "x"},     std::pair{TokenType::PLUS, "+"},
    //         std::pair{TokenType::IDENT, "y"},     std::pair{TokenType::SEMICOLON, ";"},
    //         std::pair{TokenType::RBRACE, "}"},    std::pair{TokenType::SEMICOLON, ";"},
    //         std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "result"},
    //         std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::IDENT, "add"},
    //         std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "five"},
    //         std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "ten"},
    //         std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::SEMICOLON, ";"},
    //         std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "four_and_some"},
    //         std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FLOAT, "4.2"},
    //         std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::END, ""},
    //     };

    //     Lexer l_accumulator;
    //     REQUIRE(STATUS_OK(lexer_init(&l_accumulator, input, &std_allocator)));
    //     const Fixture<Lexer> lfa{l_accumulator, lexer_deinit};
    //     REQUIRE(STATUS_OK(lexer_consume(&l_accumulator)));
    //     const ArrayList* accumulated_tokens = &l_accumulator.token_accumulator;

    //     Lexer l;
    //     REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
    //     const Fixture<Lexer> lfb{l, lexer_deinit};

    //     for (size_t i = 0; i < expecteds.size(); i++) {
    //         const auto& [t, s] = expecteds[i];
    //         const auto token   = lexer_next_token(&l);
    //         Token      accumulated_token;
    //         REQUIRE(STATUS_OK(array_list_get(accumulated_tokens, i, &accumulated_token)));

    //         REQUIRE(t == token.type);
    //         REQUIRE(t == accumulated_token.type);
    //         REQUIRE(slice_equals_str_z(&token.slice, s));
    //         REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
    //     }
    // }
}
