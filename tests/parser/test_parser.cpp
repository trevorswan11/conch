#include "catch_amalgamated.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "parser_helpers.hpp"

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/float.h"
#include "ast/expressions/function.h"
#include "ast/expressions/if.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/string.h"
#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/math.h"
#include "util/status.h"
}

TEST_CASE("Declarations") {
    SECTION("Var statements") {
        const char*   input = "var x := 5;\n"
                              "// var x := 5;\n"
                              "var y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto                     ast                  = pf.ast();
        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        REQUIRE(ast->statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));
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

        check_parse_errors(pf.parser(), expected_errors);
        REQUIRE(pf.ast()->statements.length == 0);
    }

    SECTION("Var and const statements") {
        const char*   input = "var x := 5;\n"
                              "const y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto                     ast                  = pf.ast();
        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        std::vector<bool>        is_const             = {false, true, false};
        REQUIRE(ast->statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));
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
        check_parse_errors(pf.parser(), {}, true);

        std::vector<std::string>     expected_identifiers = {"x", "z", "y", "foobar", "baz", "boo"};
        std::vector<bool>            is_const     = {false, false, true, false, false, true};
        std::vector<bool>            is_nullable  = {false, false, false, false, true, false};
        std::vector<bool>            is_primitive = {true, true, true, false, false, false};
        std::vector<TypeTag>         tags         = {TypeTag::EXPLICIT,
                                                     TypeTag::EXPLICIT,
                                                     TypeTag::EXPLICIT,
                                                     TypeTag::IMPLICIT,
                                                     TypeTag::EXPLICIT,
                                                     TypeTag::EXPLICIT};
        std::vector<ExplicitTypeTag> explicit_tags(tags.size(), ExplicitTypeTag::EXPLICIT_IDENT);
        std::vector<std::string>     type_type_literals = {token_type_name(TokenType::INT_TYPE),
                                                           token_type_name(TokenType::UINT_TYPE),
                                                           token_type_name(TokenType::BOOL_TYPE),
                                                           {},
                                                           token_type_name(TokenType::IDENT),
                                                           token_type_name(TokenType::IDENT)};

        auto                     ast                 = pf.ast();
        std::vector<std::string> expected_type_names = {
            "int", "uint", "bool", {}, "LongNum", "Conch"};
        REQUIRE(ast->statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));
            test_decl_statement(stmt,
                                is_const[i],
                                expected_identifiers[i],
                                is_nullable[i],
                                is_primitive[i],
                                type_type_literals[i],
                                tags[i],
                                explicit_tags[i],
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
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 4);

        Statement* stmt;
        for (size_t i = 0; i < ast->statements.length; i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));
            Slice literal = stmt->base.vtable->token_literal((Node*)stmt);
            REQUIRE(slice_equals_str_z(&literal, "return"));
        }
    }

    SECTION("Returns w/o sentinel semicolon") {
        const char*   input = "return 5";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);
    }
}

TEST_CASE("Identifier Expressions") {
    SECTION("Single arbitrary identifier") {
        const char*   input = "foobar;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));

        Node* node    = (Node*)stmt;
        Slice literal = node->vtable->token_literal(node);
        REQUIRE(slice_equals_str_z(&literal, "foobar"));

        ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
        test_identifier_expression(expr_stmt->expression, "foobar");
    }
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
        check_parse_errors(pf.parser(), {"SIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
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
        check_parse_errors(pf.parser(), {"UNSIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
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
            check_parse_errors(pf.parser(), {}, true);

            auto ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
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
            check_parse_errors(pf.parser(), {}, true);

            auto ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
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
            {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
            {"(5 + 5) * 2", "((5 + 5) * 2)"},
            {"2 / (5 + 5)", "(2 / (5 + 5))"},
            {"-(5 + 5)", "(-(5 + 5))"},
            {"!(true == true)", "(!(true == true))"},
        };

        for (const auto& t : cases) {
            ParserFixture pf(t.input);
            check_parse_errors(pf.parser(), {}, true);

            StringBuilder actual_builder;
            REQUIRE(STATUS_OK(string_builder_init(&actual_builder, t.expected.length())));
            REQUIRE(STATUS_OK(ast_reconstruct(pf.ast(), &actual_builder)));

            MutSlice actual;
            REQUIRE(STATUS_OK(string_builder_to_string(&actual_builder, &actual)));
            REQUIRE(t.expected == actual.ptr);
            free(actual.ptr);
        }
    }
}

TEST_CASE("Bool expressions") {
    SECTION("Basic expressions") {
        const char*   input = "true;\n"
                              "false;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 2);

        const bool  expected_values[]   = {true, false};
        const char* expected_literals[] = {"true", "false"};

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));

            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            test_bool_expression(expr_stmt->expression, expected_values[i], expected_literals[i]);
        }
    }
}

TEST_CASE("String expressions") {
    SECTION("Single-line strings") {
        const char*   input = "\"This is a string\";\n"
                              "\"Hello, 'World'!\";\n"
                              "\"\";";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 3);

        const std::string expected_strings[]  = {"This is a string", "Hello, 'World'!", ""};
        const std::string expected_literals[] = {
            "\"This is a string\"",
            "\"Hello, 'World'!\"",
            "\"\"",
        };

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));

            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            test_string_expression(
                expr_stmt->expression, expected_strings[i], expected_literals[i]);
        }
    }

    SECTION("Multiline strings") {
        const char*   input = "\\\\This is a string\n"
                              ";"
                              "\\\\Hello, 'World'!\n"
                              "\\\\\n"
                              ";"
                              "\\\\\n"
                              ";";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 3);

        const std::string expected_strings[]  = {"This is a string", "Hello, 'World'!\n", ""};
        const std::string expected_literals[] = {
            "This is a string",
            "Hello, 'World'!\n\\\\",
            "",
        };

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, &stmt)));

            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            test_string_expression(
                expr_stmt->expression, expected_strings[i], expected_literals[i]);
        }
    }
}

TEST_CASE("Conditional expressions") {
    SECTION("If without alternate") {
        const char*   input = "if (x < y) { x }";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
        ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;

        IfExpression* if_expr = (IfExpression*)expr_stmt->expression;
        REQUIRE_FALSE(if_expr->alternate);

        InfixExpression* condition = (InfixExpression*)if_expr->condition;
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        BlockStatement* consequence = (BlockStatement*)if_expr->consequence;
        REQUIRE(consequence->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence->statements, 0, &stmt)));
        expr_stmt = (ExpressionStatement*)stmt;
        test_identifier_expression(expr_stmt->expression, "x");
    }

    SECTION("If with alternate") {
        const char*   input = "if (x < y) { x } else { y }";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
        ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;

        IfExpression* if_expr = (IfExpression*)expr_stmt->expression;
        REQUIRE(if_expr->alternate);

        InfixExpression* condition = (InfixExpression*)if_expr->condition;
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        BlockStatement* consequence = (BlockStatement*)if_expr->consequence;
        REQUIRE(consequence->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence->statements, 0, &stmt)));
        expr_stmt = (ExpressionStatement*)stmt;
        test_identifier_expression(expr_stmt->expression, "x");

        BlockStatement* alternate = (BlockStatement*)if_expr->alternate;
        REQUIRE(alternate->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&alternate->statements, 0, &stmt)));
        expr_stmt = (ExpressionStatement*)stmt;
        test_identifier_expression(expr_stmt->expression, "y");
    }

    SECTION("If/Else with non-block assignment") {
        const char*   input = "const val := if (x >= y) 1 else 2;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
        test_decl_statement(stmt, true, "val");
        DeclStatement* decl_stmt = (DeclStatement*)stmt;
        REQUIRE(decl_stmt->type->type.tag == IMPLICIT);

        IfExpression* if_expr = (IfExpression*)decl_stmt->value;
        REQUIRE(if_expr->alternate);

        InfixExpression* condition = (InfixExpression*)if_expr->condition;
        REQUIRE(condition->op == TokenType::GTEQ);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        ExpressionStatement* consequence = (ExpressionStatement*)if_expr->consequence;
        test_number_expression<int64_t>(consequence->expression, "1", 1);

        ExpressionStatement* alternate = (ExpressionStatement*)if_expr->alternate;
        test_number_expression<int64_t>(alternate->expression, "2", 2);
    }

    SECTION("If/Else with return statement") {
        const char*   input = "if (true) return 1; else return 2;";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
        ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;

        IfExpression* if_expr = (IfExpression*)expr_stmt->expression;
        REQUIRE(if_expr->alternate);

        BoolLiteralExpression* condition = (BoolLiteralExpression*)if_expr->condition;
        REQUIRE(condition->value);

        ReturnStatement* consequence = (ReturnStatement*)if_expr->consequence;
        test_number_expression<int64_t>(consequence->value, "1", 1);

        ReturnStatement* alternate = (ReturnStatement*)if_expr->alternate;
        test_number_expression<int64_t>(alternate->value, "2", 2);
    }

    SECTION("If/Else with nested ifs and terminal else") {
        const char* input = "if (x < y) {return -1;} else if (x > y) {return 1;} else {return 0;}";
        ParserFixture pf(input);
        check_parse_errors(pf.parser(), {}, true);

        auto ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));
        ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;

        // Verify the first condition
        IfExpression* if_expr = (IfExpression*)expr_stmt->expression;
        REQUIRE(if_expr->alternate);

        InfixExpression* condition = (InfixExpression*)if_expr->condition;
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        // Verify the first consequence
        BlockStatement* consequence_one = (BlockStatement*)if_expr->consequence;
        REQUIRE(consequence_one->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence_one->statements, 0, &stmt)));
        ReturnStatement*  return_stmt = (ReturnStatement*)stmt;
        PrefixExpression* neg_num     = (PrefixExpression*)return_stmt->value;
        REQUIRE(neg_num->token.type == TokenType::MINUS);
        test_number_expression<int64_t>(neg_num->rhs, "1", 1);

        // Verify the first alternate and its condition
        ExpressionStatement* alternate_one = (ExpressionStatement*)if_expr->alternate;
        if_expr                            = (IfExpression*)alternate_one->expression;
        REQUIRE(if_expr->alternate);

        condition = (InfixExpression*)if_expr->condition;
        REQUIRE(condition->op == TokenType::GT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        // Verify the second consequence
        BlockStatement* consequence_two = (BlockStatement*)if_expr->consequence;
        REQUIRE(consequence_two->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence_two->statements, 0, &stmt)));
        return_stmt = (ReturnStatement*)stmt;
        test_number_expression<int64_t>(return_stmt->value, "1", 1);

        // Verify the second alternate
        BlockStatement* alternate_two = (BlockStatement*)if_expr->alternate;
        REQUIRE(alternate_two->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&alternate_two->statements, 0, &stmt)));
        return_stmt = (ReturnStatement*)stmt;
        test_number_expression<int64_t>(return_stmt->value, "0", 0);
    }
}

TEST_CASE("Function literals") {
    struct TestParameter {
        std::string name;

        bool        nullable     = false;
        bool        primitive    = true;
        std::string type_literal = token_type_name(TokenType::INT_TYPE);
        TypeTag     tag          = TypeTag::EXPLICIT;
        std::string type_name    = "int";

        std::optional<int64_t> default_value = std::nullopt;
    };

    const auto test_parameters = [](ArrayList* actuals, std::vector<TestParameter> expecteds) {
        REQUIRE(actuals->length == expecteds.size());
        for (size_t i = 0; i < actuals->length; i++) {
            Parameter parameter;
            REQUIRE(STATUS_OK(array_list_get(actuals, i, &parameter)));
            TestParameter expected_parameter = expecteds[i];

            test_identifier_expression((Expression*)parameter.ident, expected_parameter.name);
            test_type_expression((Expression*)parameter.type,
                                 expected_parameter.nullable,
                                 expected_parameter.primitive,
                                 expected_parameter.type_literal,
                                 expected_parameter.tag,
                                 ExplicitTypeTag::EXPLICIT_IDENT,
                                 expected_parameter.type_name);

            if (expected_parameter.default_value.has_value()) {
                test_number_expression<int64_t>(
                    parameter.default_value, NULL, expected_parameter.default_value.value());
            } else {
                REQUIRE_FALSE(parameter.default_value);
                REQUIRE_FALSE(expected_parameter.default_value.has_value());
            }
        }
    };

    SECTION("Functions as types") {
        SECTION("Correct declaration") {
            std::vector<TestParameter> expected_params = {TestParameter{"a"}, TestParameter{"b"}};

            const char*   input = "var add: fn(a: int, b: int);";
            ParserFixture pf(input);
            check_parse_errors(pf.parser(), {}, true);
            auto ast = pf.ast();

            REQUIRE(ast->statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));

            test_decl_statement(stmt,
                                false,
                                "add",
                                false,
                                false,
                                {},
                                TypeTag::EXPLICIT,
                                ExplicitTypeTag::EXPLICIT_FN,
                                {});
            DeclStatement* decl_stmt = (DeclStatement*)stmt;
            REQUIRE_FALSE(decl_stmt->value);

            TypeExpression* type_expr     = (TypeExpression*)decl_stmt->type;
            ExplicitType    explicit_type = type_expr->type.variant.explicit_type;
            ArrayList       parameters    = explicit_type.variant.fn_type_params;
            test_parameters(&parameters, expected_params);
        }

        SECTION("Incorrect declaration with first default") {
            const char*   input = "var add: fn(a: int = 1, b: int);";
            ParserFixture pf(input);
            check_parse_errors(pf.parser(), {"ILLEGAL_DEFAULT_FUNCTION_PARAMETER [Ln 1, Col 8]"});
        }

        SECTION("Incorrect declaration with second default") {
            const char*   input = "var add: fn(a: int, b: int = 2);";
            ParserFixture pf(input);
            check_parse_errors(pf.parser(), {"ILLEGAL_DEFAULT_FUNCTION_PARAMETER [Ln 1, Col 8]"});
        }

        SECTION("Incorrect declaration with both default") {
            const char*   input = "const add: fn(a: int = -345, b: uint = 209u);";
            ParserFixture pf(input);
            check_parse_errors(pf.parser(),
                               {"ILLEGAL_DEFAULT_FUNCTION_PARAMETER [Ln 1, Col 10]",
                                "CONST_DECL_MISSING_VALUE [Ln 1, Col 1]"});
        }
    }

    SECTION("Parameter allocation") {
        struct TestCase {
            const char*                input;
            std::vector<TestParameter> expected_params;
        };

        const TestCase cases[] = {
            {"fn() {};", {}},
            {"fn(x: int) {};", {TestParameter{"x"}}},
            {"fn(x: int, y: int, z: int) {};",
             {TestParameter{"x"}, TestParameter{"y"}, TestParameter{"z"}}},
            {"fn(x: int, y: int = 2, z: int) {};",
             {TestParameter{"x"},
              TestParameter{.name = "y", .default_value = 2},
              TestParameter{"z"}}},
            {"fn(x: int, y: int, z: int = 3) {};",
             {TestParameter{"x"},
              TestParameter{"y"},
              TestParameter{.name = "z", .default_value = 3}}},
        };

        for (const auto& t : cases) {
            ParserFixture pf(t.input);
            check_parse_errors(pf.parser(), {}, true);
            auto ast = pf.ast();

            REQUIRE(ast->statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));

            ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
            FunctionExpression*  function  = (FunctionExpression*)expr_stmt->expression;
            REQUIRE(function->body->statements.length == 0);

            ArrayList parameters = function->parameters;
            test_parameters(&parameters, t.expected_params);
        }
    }
}
