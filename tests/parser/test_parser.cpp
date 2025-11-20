#include "catch_amalgamated.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/float.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/statement.h"

#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/math.h"
#include "util/status.h"
}

FileIO stdio = file_io_std();

struct ParserFixture {
    ParserFixture(const char* input) {
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
        REQUIRE(STATUS_OK(lexer_consume(&l)));

        REQUIRE(STATUS_OK(ast_init(&ast, standard_allocator)));

        REQUIRE(STATUS_OK(parser_init(&p, &l, &stdio, standard_allocator)));
        REQUIRE(STATUS_OK(parser_consume(&p, &ast)));
    }

    ~ParserFixture() {
        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }

    Parser p;
    AST    ast;
    Lexer  l;
};

static inline void check_parse_errors(Parser*                  p,
                                      std::vector<std::string> expected_errors,
                                      bool                     print_anyways = false) {
    const ArrayList* actual_errors = &p->errors;
    MutSlice         error;

    if (actual_errors->length == 0 && expected_errors.size() == 0) {
        return;
    } else if (print_anyways && actual_errors->length != 0) {
        for (size_t i = 0; i < actual_errors->length; i++) {
            REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
            std::cerr << "Parser error: " << error.ptr << "\n";
        }
    }

    REQUIRE(actual_errors->length == expected_errors.size());

    for (size_t i = 0; i < actual_errors->length; i++) {
        REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
        std::string expected = expected_errors[i];
        REQUIRE(mut_slice_equals_str_z(&error, expected.c_str()));
    }
}

static inline void
test_decl_statement(Statement* stmt, bool expect_const, const char* expected_ident) {
    Node* stmt_node = (Node*)stmt;
    Slice literal   = stmt_node->vtable->token_literal(stmt_node);
    REQUIRE(slice_equals_str_z(&literal, expect_const ? "const" : "var"));

    DeclStatement*        decl_stmt = (DeclStatement*)stmt;
    IdentifierExpression* ident     = decl_stmt->ident;
    Node*                 node      = (Node*)ident;
    literal                         = node->vtable->token_literal(node);

    REQUIRE(slice_equals_str_z(&literal, token_type_name(TokenType::IDENT)));
    REQUIRE(mut_slice_equals_str_z(&decl_stmt->ident->name, expected_ident));
}

static inline void test_decl_statement(Statement*  stmt,
                                       bool        expect_const,
                                       const char* expected_ident,
                                       bool        expect_nullable,
                                       bool        expect_primitive,
                                       const char* expected_type_literal,
                                       TypeTag     expected_tag,
                                       const char* expected_type_name) {
    Node* stmt_node = (Node*)stmt;
    Slice literal   = stmt_node->vtable->token_literal(stmt_node);
    REQUIRE(slice_equals_str_z(&literal, expect_const ? "const" : "var"));

    DeclStatement*        decl_stmt = (DeclStatement*)stmt;
    IdentifierExpression* ident     = decl_stmt->ident;
    Node*                 node      = (Node*)ident;
    literal                         = node->vtable->token_literal(node);

    REQUIRE(slice_equals_str_z(&literal, token_type_name(TokenType::IDENT)));
    REQUIRE(mut_slice_equals_str_z(&ident->name, expected_ident));

    TypeExpression* type_expr = (TypeExpression*)decl_stmt->type;
    Type            type      = type_expr->type;

    REQUIRE(type.tag == expected_tag);
    if (type.tag == TypeTag::EXPLICIT) {
        const ExplicitType explicit_type = type.variant.explicit_type;
        REQUIRE(explicit_type.nullable == expect_nullable);
        REQUIRE(explicit_type.primitive == expect_primitive);

        ident   = explicit_type.identifier;
        node    = (Node*)ident;
        literal = node->vtable->token_literal(node);

        REQUIRE(slice_equals_str_z(&literal, expected_type_literal));
        REQUIRE(mut_slice_equals_str_z(&ident->name, expected_type_name));
    } else {
        REQUIRE_FALSE(expect_nullable);
        REQUIRE_FALSE(expect_primitive);
        REQUIRE_FALSE(expected_type_name);
    }
}

TEST_CASE("Declarations") {
    SECTION("Var statements") {
        const char*   input = "var x := 5;\n"
                              "var y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        REQUIRE(pf.ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            test_decl_statement(stmt, false, expected_identifiers[i]);
        }
    }

    SECTION("Var statements with errors") {
        const char*   input = "var x 5;\n"
                              "var = 10;\n"
                              "var 838383;\n"
                              "var z := 6";
        ParserFixture pf(input);

        std::vector<std::string> expected_errors = {
            "Expected token COLON, found INT_10 [Ln 1, Col 7]",
            "Expected token IDENT, found ASSIGN [Ln 2, Col 5]",
            "No prefix parse function for ASSIGN found [Ln 2, Col 5]",
            "Expected token IDENT, found INT_10 [Ln 3, Col 5]",
        };

        check_parse_errors(&pf.p, expected_errors);
        REQUIRE(pf.ast.statements.length == 0);
    }

    SECTION("Var and const statements") {
        const char*   input = "var x := 5;\n"
                              "const y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        std::vector<bool>        is_const             = {false, true, false};
        REQUIRE(pf.ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            test_decl_statement(stmt, is_const[i], expected_identifiers[i]);
        }
    }

    SECTION("Complex typed decl statements") {
        const char*   input = "var x: int = 5;\n"
                              "var z: uint;\n"
                              "const y: bool = 10;\n"
                              "var foobar := 838383;\n"
                              "var baz: ?LongNum = 838383;\n"
                              "const boo: Conch = 2;\n";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);

        std::vector<const char*> expected_identifiers = {"x", "z", "y", "foobar", "baz", "boo"};
        std::vector<bool>        is_const             = {false, false, true, false, false, true};
        std::vector<bool>        is_nullable          = {false, false, false, false, true, false};
        std::vector<bool>        is_primitive         = {true, true, true, false, false, false};
        std::vector<TypeTag>     tags                 = {TypeTag::EXPLICIT,
                                                         TypeTag::EXPLICIT,
                                                         TypeTag::EXPLICIT,
                                                         TypeTag::IMPLICIT,
                                                         TypeTag::EXPLICIT,
                                                         TypeTag::EXPLICIT};
        std::vector<const char*> type_type_literals   = {token_type_name(TokenType::INT_TYPE),
                                                         token_type_name(TokenType::UINT_TYPE),
                                                         token_type_name(TokenType::BOOL_TYPE),
                                                         NULL,
                                                         token_type_name(TokenType::IDENT),
                                                         token_type_name(TokenType::IDENT)};
        std::vector<const char*> expected_type_names  = {
            "int", "uint", "bool", NULL, "LongNum", "Conch"};
        REQUIRE(pf.ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            test_decl_statement(stmt,
                                is_const[i],
                                expected_identifiers[i],
                                is_nullable[i],
                                is_primitive[i],
                                type_type_literals[i],
                                tags[i],
                                expected_type_names[i]);
        }
    }
}

TEST_CASE("Return statements") {
    SECTION("Happy returns") {
        const char*   input = "return;\n"
                              "return 5;\n"
                              "return 10;\n"
                              "return 993322;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);
        REQUIRE(pf.ast.statements.length == 4);

        Statement* stmt;
        for (size_t i = 0; i < pf.ast.statements.length; i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            Slice literal = stmt->base.vtable->token_literal((Node*)stmt);
            REQUIRE(slice_equals_str_z(&literal, "return"));
        }
    }

    SECTION("Returns w/o sentinel semicolon") {
        const char*   input = "return 5";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {}, true);
    }
}

TEST_CASE("Identifier Expressions") {
    SECTION("Single arbitrary identifier") {
        const char*   input = "foobar;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);
        REQUIRE(pf.ast.statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));

        Node* node    = (Node*)stmt;
        Slice literal = node->vtable->token_literal(node);
        REQUIRE(slice_equals_str_z(&literal, "foobar"));

        ExpressionStatement*  expr_stmt = (ExpressionStatement*)stmt;
        IdentifierExpression* ident     = (IdentifierExpression*)expr_stmt->expression;
        REQUIRE(ident->token_type == TokenType::IDENT);
        REQUIRE(mut_slice_equals_str_z(&ident->name, "foobar"));
    }
}

template <typename T>
static inline void
test_number_expression(Expression* expression, const char* expected_literal, T expected_value) {
    if constexpr (std::is_same_v<T, double>) {
        FloatLiteralExpression* f = (FloatLiteralExpression*)expression;
        REQUIRE(f->value == expected_value);
        Node* f_node = (Node*)f;
        if (expected_literal) {
            Slice literal = f_node->vtable->token_literal(f_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else if constexpr (std::is_same_v<T, int64_t>) {
        IntegerLiteralExpression* i = (IntegerLiteralExpression*)expression;
        REQUIRE(i->value == expected_value);
        Node* i_node = (Node*)i;
        if (expected_literal) {
            Slice literal = i_node->vtable->token_literal(i_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        UnsignedIntegerLiteralExpression* i = (UnsignedIntegerLiteralExpression*)expression;
        REQUIRE(i->value == expected_value);
        Node* i_node = (Node*)i;
        if (expected_literal) {
            Slice literal = i_node->vtable->token_literal(i_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else {
        REQUIRE(false);
    }
}

template <typename T>
static inline void
test_number_expression(const char* input, const char* expected_literal, T expected_value) {
    ParserFixture pf(input);

    check_parse_errors(&pf.p, {}, true);
    REQUIRE(pf.ast.statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));

    ExpressionStatement* expr = (ExpressionStatement*)stmt;
    test_number_expression<T>(expr->expression, expected_literal, expected_value);
}

TEST_CASE("Number-based expressions") {
    SECTION("Signed integer bases") {
        test_number_expression<int64_t>("5;", "5", 5);
        test_number_expression<int64_t>("0b10011101101;", "0b10011101101", 0b10011101101);
        test_number_expression<int64_t>("0o1234567;", "0o1234567", 342391);
        test_number_expression<int64_t>("0xFF8a91d;", "0xFF8a91d", 0xFF8a91d);
    }

    SECTION("Signed integer overflow") {
        const char*   input = "0xFFFFFFFFFFFFFFFF";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {"SIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
    }

    SECTION("Unsigned integer bases") {
        test_number_expression<uint64_t>("5u;", "5u", 5);
        test_number_expression<uint64_t>("0b10011101101u;", "0b10011101101u", 0b10011101101);
        test_number_expression<uint64_t>("0o1234567U;", "0o1234567U", 342391);
        test_number_expression<uint64_t>("0xFF8a91du;", "0xFF8a91du", 0xFF8a91d);

        test_number_expression<uint64_t>(
            "0xFFFFFFFFFFFFFFFFu;", "0xFFFFFFFFFFFFFFFFu", 0xFFFFFFFFFFFFFFFF);
    }

    SECTION("Unsigned integer overflow") {
        const char*   input = "0x10000000000000000u";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {"UNSIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
    }

    SECTION("Floating points") {
        test_number_expression<double>("1023.0;", "1023.0", 1023.0);
        test_number_expression<double>("1023.234612;", "1023.234612", 1023.234612);
        test_number_expression<double>("1023.234612e234;", "1023.234612e234", 1023.234612e234);
    }
}

TEST_CASE("Basic prefix / infix expressions") {
    SECTION("Prefix expressions") {
        struct TestCase {
            const char*                             input;
            TokenType                               op;
            std::variant<int64_t, uint64_t, double> value;
        };

        const TestCase cases[] = {
            {"!5", TokenType::BANG, 5ll},
            {"-15u", TokenType::MINUS, 15ull},
            {"!3.4", TokenType::BANG, 3.4},
            {"~0b101101", TokenType::NOT, 0b101101ll},
            {"!1.2345e100", TokenType::BANG, 1.2345e100},
        };

        for (const auto& t : cases) {
            ParserFixture pf(t.input);
            check_parse_errors(&pf.p, {}, true);

            REQUIRE(pf.ast.statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));
            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            PrefixExpression*    expr      = (PrefixExpression*)expr_stmt->expression;

            REQUIRE(expr->token.type == t.op);
            if (std::holds_alternative<int64_t>(t.value)) {
                test_number_expression<int64_t>(expr->rhs, NULL, std::get<int64_t>(t.value));
            } else if (std::holds_alternative<uint64_t>(t.value)) {
                test_number_expression<uint64_t>(expr->rhs, NULL, std::get<uint64_t>(t.value));
            } else if (std::holds_alternative<double>(t.value)) {
                test_number_expression<double>(expr->rhs, NULL, std::get<double>(t.value));
            } else {
                REQUIRE(false);
            }
        }
    }

    SECTION("Infix expressions") {
        struct TestCase {
            const char*                             input;
            std::variant<int64_t, uint64_t, double> lvalue;
            TokenType                               op;
            std::variant<int64_t, uint64_t, double> rvalue;
        };

        const TestCase cases[] = {
            {"5 + 5;", 5ll, TokenType::PLUS, 5ll},
            {"5 - 5;", 5ll, TokenType::MINUS, 5ll},
            {"5.0 * 5.2;", 5.0, TokenType::STAR, 5.2},
            {"4.9e2 / 5.1e3;", 4.9e2, TokenType::SLASH, 5.1e3},
            {"0x231 % 0xF;", 0x231, TokenType::PERCENT, 0xF},
            {"5 < 5;", 5ll, TokenType::LT, 5ll},
            {"5 <= 5;", 5ll, TokenType::LTEQ, 5ll},
            {"5 > 5;", 5ll, TokenType::GT, 5ll},
            {"5 >= 5;", 5ll, TokenType::GTEQ, 5ll},
            {"5 == 5;", 5ll, TokenType::EQ, 5ll},
            {"5 != 5;", 5ll, TokenType::NEQ, 5ll},
            {"0b10111u & 0b10110u;", 0b10111ull, TokenType::AND, 0b10110ull},
            {"0b10111u | 0b10110u;", 0b10111ull, TokenType::OR, 0b10110ull},
            {"0b10111u ^ 0b10110u;", 0b10111ull, TokenType::XOR, 0b10110ull},
            {"0b10111u >> 5u;", 0b10111ull, TokenType::SHR, 5ull},
            {"0b10111u << 4u;", 0b10111ull, TokenType::SHL, 4ull},
        };

        for (const auto& t : cases) {
            ParserFixture pf(t.input);
            check_parse_errors(&pf.p, {}, true);

            REQUIRE(pf.ast.statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));
            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            InfixExpression*     expr      = (InfixExpression*)expr_stmt->expression;

            const std::pair<std::variant<int64_t, uint64_t, double>, Expression*> pairs[] = {
                {t.lvalue, expr->lhs},
                {t.rvalue, expr->rhs},
            };

            REQUIRE(expr->op == t.op);
            for (const auto& p : pairs) {
                if (std::holds_alternative<int64_t>(p.first)) {
                    test_number_expression<int64_t>(p.second, NULL, std::get<int64_t>(p.first));
                } else if (std::holds_alternative<uint64_t>(p.first)) {
                    test_number_expression<uint64_t>(p.second, NULL, std::get<uint64_t>(p.first));
                } else if (std::holds_alternative<double>(p.first)) {
                    test_number_expression<double>(p.second, NULL, std::get<double>(p.first));
                } else {
                    REQUIRE(false);
                }
            }
        }
    }

    SECTION("Operator precedence parsing") {
        struct TestCase {
            const char* input;
            std::string expected;
        };

        const TestCase cases[] = {
            {"-a * b", "((-a) * b)"},
            {"!-a", "(!(-a))"},
            {"a + b + c", "((a + b) + c)"},
            {"a + b - c", "((a + b) - c)"},
            {"a * b * c", "((a * b) * c)"},
            {"a * b / c", "((a * b) / c)"},
            {"a + b / c", "(a + (b / c))"},
            {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
            {"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
            {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
            {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
            {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
        };

        for (const auto& t : cases) {
            ParserFixture pf(t.input);
            check_parse_errors(&pf.p, {}, true);

            StringBuilder actual_builder;
            REQUIRE(STATUS_OK(string_builder_init(&actual_builder, t.expected.length())));
            REQUIRE(STATUS_OK(ast_reconstruct(&pf.ast, &actual_builder)));

            MutSlice actual;
            REQUIRE(STATUS_OK(string_builder_to_string(&actual_builder, &actual)));
            REQUIRE(t.expected == actual.ptr);
            free(actual.ptr);
        }
    }
}
