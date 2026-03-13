#include <span>

#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/call.hpp"
#include "ast/expressions/index.hpp"
#include "ast/expressions/infix.hpp"
#include "ast/expressions/prefix.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/scope_resolve.hpp"

#include "lexer/operators.hpp"

namespace conch::tests {

namespace helpers {

template <ast::LeafNode N, ast::LeafNode L, ast::LeafNode R>
auto test_infix_expr(std::string_view input, L&& lhs, TokenType op, R&& rhs) -> void {
    Lexer       l{input};
    const auto& start_token = l.advance();
    test_expr_stmt(input,
                   N{start_token, make_box<L>(std::move(lhs)), op, make_box<R>(std::move(rhs))});
}

template <ast::LeafNode N> auto test_infix_op_list(std::span<const Operator> ops) -> void {
    for (const auto& op : ops) {
        const auto input = fmt::format("a {} b;", op.first);
        helpers::test_infix_expr<N>(input, ident_from("a"), op.second, ident_from("b"));
    }
}

template <ast::LeafNode L, ast::LeafNode R>
void test_binary_expr(std::string_view input, L&& lhs, TokenType op, R&& rhs) {
    helpers::test_infix_expr<ast::BinaryExpression>(
        input, std::forward<L>(lhs), op, std::forward<R>(rhs));
}

void test_binary_expr(std::string_view        input,
                      const Token&            start_token,
                      ast::BinaryExpression&& expected) {
    test_expr_stmt(input, start_token, std::move(expected));
}

} // namespace helpers

TEST_CASE("Assignment expressions") {
    const auto ops = std::array{
        operators::ASSIGN,
        operators::PLUS_ASSIGN,
        operators::MINUS_ASSIGN,
        operators::STAR_ASSIGN,
        operators::SLASH_ASSIGN,
        operators::PERCENT_ASSIGN,
        operators::BW_AND_ASSIGN,
        operators::BW_OR_ASSIGN,
        operators::SHL_ASSIGN,
        operators::SHR_ASSIGN,
        operators::NOT_ASSIGN,
        operators::XOR_ASSIGN,
    };
    helpers::test_infix_op_list<ast::AssignmentExpression>(ops);
}

TEST_CASE("Binary expressions") {
    const auto ops = std::array{
        operators::PLUS,
        operators::MINUS,
        operators::STAR,
        operators::SLASH,
        operators::PERCENT,
        operators::LT,
        operators::LT_EQ,
        operators::GT,
        operators::GT_EQ,
        operators::EQ,
        operators::NEQ,
        operators::BOOLEAN_AND,
        operators::BOOLEAN_OR,
        operators::BW_AND,
        operators::BW_OR,
        operators::XOR,
        operators::SHR,
        operators::SHL,
    };
    helpers::test_infix_op_list<ast::BinaryExpression>(ops);
}

TEST_CASE("Miscellaneous infixes") {
    SECTION("Range expressions") {
        const auto ops = std::array{
            operators::DOT_DOT,
            operators::DOT_DOT_EQ,
        };
        helpers::test_infix_op_list<ast::RangeExpression>(ops);
    }

    SECTION("Dot expression") {
        const auto ops = std::array{operators::DOT};
        helpers::test_infix_op_list<ast::DotExpression>(ops);
    }
}

const Token a{TokenType::IDENT, "a"};
const Token b{TokenType::IDENT, "b"};
const Token c{TokenType::IDENT, "c"};

TEST_CASE("Numerical precedence") {
    SECTION("Add/sub and mul/div") {
        helpers::test_binary_expr(
            "a + b * c;",
            helpers::ident_from(a),
            TokenType::PLUS,
            ast::BinaryExpression{
                b, helpers::make_ident(b), TokenType::STAR, helpers::make_ident(c)});

        helpers::test_binary_expr(
            "a + b / c;",
            helpers::ident_from(a),
            TokenType::PLUS,
            ast::BinaryExpression{
                b, helpers::make_ident(b), TokenType::SLASH, helpers::make_ident(c)});

        helpers::test_binary_expr(
            "a * b - c;",
            ast::BinaryExpression{
                a, helpers::make_ident(a), TokenType::STAR, helpers::make_ident(b)},
            TokenType::MINUS,
            helpers::ident_from(c));

        helpers::test_binary_expr(
            "a + b - c;",
            ast::BinaryExpression{
                a, helpers::make_ident(a), TokenType::PLUS, helpers::make_ident(b)},
            TokenType::MINUS,
            helpers::ident_from(c));
    }

    SECTION("Grouping") {
        helpers::test_binary_expr(
            "(a + b) * c;",
            Token{TokenType::LPAREN, "("},
            ast::BinaryExpression{
                a,
                make_box<ast::BinaryExpression>(
                    a, helpers::make_ident(a), TokenType::PLUS, helpers::make_ident(b)),
                TokenType::STAR,
                helpers::make_ident(c)});
    }
}

const Token d{TokenType::IDENT, "d"};

TEST_CASE("Bitwise operations") {
    helpers::test_binary_expr(
        "a & b | c;",
        ast::BinaryExpression{a, helpers::make_ident(a), TokenType::BW_AND, helpers::make_ident(b)},
        TokenType::BW_OR,
        helpers::ident_from(c));

    helpers::test_binary_expr(
        "a | b & c;",
        helpers::ident_from(a),
        TokenType::BW_OR,
        ast::BinaryExpression{
            b, helpers::make_ident(b), TokenType::BW_AND, helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b & c;",
        helpers::ident_from(a),
        TokenType::BW_OR,
        ast::BinaryExpression{
            b, helpers::make_ident(b), TokenType::BW_AND, helpers::make_ident(c)});

    helpers::test_binary_expr(
        "(a | b) & c;",
        Token{TokenType::LPAREN, "("},
        ast::BinaryExpression{
            a,
            make_box<ast::BinaryExpression>(
                a, helpers::make_ident(a), TokenType::BW_OR, helpers::make_ident(b)),
            TokenType::BW_AND,
            helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b ^ c & d;",
        ast::BinaryExpression{a, helpers::make_ident(a), TokenType::BW_OR, helpers::make_ident(b)},
        TokenType::XOR,
        ast::BinaryExpression{
            c, helpers::make_ident(c), TokenType::BW_AND, helpers::make_ident(d)});
}

const Token e{TokenType::IDENT, "e"};

TEST_CASE("Boolean operations") {
    helpers::test_binary_expr(
        "a < b and c > d;",
        ast::BinaryExpression{a, helpers::make_ident(a), TokenType::LT, helpers::make_ident(b)},
        TokenType::BOOLEAN_AND,
        ast::BinaryExpression{c, helpers::make_ident(c), TokenType::GT, helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a <= b or c == d and e;",
        ast::BinaryExpression{
            a,
            make_box<ast::BinaryExpression>(
                a, helpers::make_ident(a), TokenType::LT_EQ, helpers::make_ident(b)),
            TokenType::BOOLEAN_OR,
            make_box<ast::BinaryExpression>(
                c, helpers::make_ident(c), TokenType::EQ, helpers::make_ident(d))},
        TokenType::BOOLEAN_AND,
        helpers::ident_from(e));
}

TEST_CASE("Prefix precedence") {
    const Token minus{operators::MINUS};
    helpers::test_binary_expr(
        "a + -b * c;",
        helpers::ident_from(a),
        TokenType::PLUS,
        ast::BinaryExpression{minus,
                              make_box<ast::UnaryExpression>(minus, helpers::make_ident(b)),
                              TokenType::STAR,
                              helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b & ~c;",
        helpers::ident_from(a),
        TokenType::BW_OR,
        ast::BinaryExpression{
            b,
            helpers::make_ident(b),
            TokenType::BW_AND,
            make_box<ast::UnaryExpression>(Token{operators::NOT}, helpers::make_ident(c))});

    helpers::test_binary_expr("a and !b;",
                              helpers::ident_from(a),
                              TokenType::BOOLEAN_AND,
                              ast::UnaryExpression{Token{operators::BANG}, helpers::make_ident(b)});
}

TEST_CASE("Call precedence") {
    helpers::test_binary_expr(
        "a and b() != c;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_AND,
        ast::BinaryExpression{b,
                              make_box<ast::CallExpression>(
                                  b, helpers::make_ident(b), std::vector<ast::CallArgument>{}),
                              TokenType::NEQ,
                              helpers::make_ident(c)});
}

TEST_CASE("Index precedence") {
    helpers::test_binary_expr(
        "a or b[3uz] == !c;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            make_box<ast::IndexExpression>(
                b,
                helpers::make_ident(b),
                make_box<ast::USizeIntegerExpression>(Token{TokenType::UZINT_10, "3uz"}, 3uz)),
            TokenType::EQ,
            make_box<ast::UnaryExpression>(Token{operators::BANG}, helpers::make_ident(c))});
}

TEST_CASE("Scope resolution precedence") {
    helpers::test_binary_expr(
        "a or b.c >= d;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            make_box<ast::DotExpression>(
                b, helpers::make_ident(b), TokenType::DOT, helpers::make_ident(c)),
            TokenType::GT_EQ,
            helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a or b::c < d;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_OR,
        ast::BinaryExpression{b,
                              make_box<ast::ScopeResolutionExpression>(
                                  b, helpers::make_ident(b), helpers::make_ident(c)),
                              TokenType::LT,
                              helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a or b.c >= d;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            make_box<ast::DotExpression>(
                b, helpers::make_ident(b), TokenType::DOT, helpers::make_ident(c)),
            TokenType::GT_EQ,
            helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a or b->c < d;",
        helpers::ident_from(a),
        TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            make_box<ast::ImplicitDereferenceExpression>(
                b, helpers::make_ident(b), TokenType::ARROW, helpers::make_ident(c)),
            TokenType::LT,
            helpers::make_ident(d)});
}

TEST_CASE("Illegal infix node") {
    helpers::test_fail("a and import std;",
                       ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        7});
}

TEST_CASE("Non-terminated infix") {
    helpers::test_fail("a and;",
                       ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        6});
    helpers::test_fail("a and", ParserDiagnostic{ParserError::INFIX_MISSING_RHS, 1, 3});
}

} // namespace conch::tests
