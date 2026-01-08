#include "catch_amalgamated.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ast_nodes.hpp"
#include "fixtures.hpp"

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/array.h"
#include "ast/expressions/assignment.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/call.h"
#include "ast/expressions/enum.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/float.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/if.h"
#include "ast/expressions/index.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/loop.h"
#include "ast/expressions/match.h"
#include "ast/expressions/namespace.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/single.h"
#include "ast/expressions/string.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/discard.h"
#include "ast/statements/expression.h"
#include "ast/statements/impl.h"
#include "ast/statements/import.h"
#include "ast/statements/jump.h"
#include "ast/statements/statement.h"

#include "lexer/token.h"
#include "util/containers/array_list.h"
#include "util/memory.h"
#include "util/status.h"
}

TEST_CASE("Precedence coverage") {
    const std::string expected = "MUL_DIV";
    REQUIRE(expected == precedence_name(Precedence::MUL_DIV));
}

TEST_CASE("Declarations") {
    SECTION("Var statements") {
        const char*         input = "var x := 5;\n"
                                    "// var x := 5;\n"
                                    "var y := 10;\n"
                                    "var foobar := 838383;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast                  = pf.ast();
        const auto  expected_identifiers = std::array{"x", "y", "foobar"};
        REQUIRE(ast->statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < std::size(expected_identifiers); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));
            test_decl_statement(stmt, false, expected_identifiers[i]);
        }
    }

    SECTION("Var statements with errors") {
        const char*         input = "var x 5;\n"
                                    "var = 10;\n"
                                    "var 838383;\n"
                                    "var z := 6";
        const ParserFixture pf{input};

        const auto expected_errors = std::to_array<std::string>({
            "Expected token COLON, found INT_10 [Ln 1, Col 7]",
            "Expected token IDENT, found ASSIGN [Ln 2, Col 5]",
            "No prefix parse function for ASSIGN found [Ln 2, Col 5]",
            "Expected token IDENT, found INT_10 [Ln 3, Col 5]",
        });

        pf.check_errors(expected_errors);
        REQUIRE(pf.ast()->statements.length == 0);
    }

    SECTION("Var and const statements") {
        const char*         input = "var x := 5;\n"
                                    "const y := 10;\n"
                                    "var foobar := 838383;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast                  = pf.ast();
        const auto  expected_identifiers = std::array{"x", "y", "foobar"};
        const auto  is_const             = std::array{false, true, false};
        REQUIRE(ast->statements.length == std::size(expected_identifiers));

        Statement* stmt;
        for (size_t i = 0; i < std::size(expected_identifiers); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));
            test_decl_statement(stmt, is_const[i], expected_identifiers[i]);
        }
    }

    SECTION("Complex typed decl statements") {
        const char*         input = "var x: int = 5;\n"
                                    "var z: uint;\n"
                                    "const y: bool = 10;\n"
                                    "var foobar := 838383;\n"
                                    "var baz: ?LongNum = 838383;\n"
                                    "const boo: Conch = 2;\n";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto expected_identifiers =
            std::to_array<std::string>({"x", "z", "y", "foobar", "baz", "boo"});
        const auto is_const     = std::array{false, false, true, false, false, true};
        const auto is_nullable  = std::array{false, false, false, false, true, false};
        const auto is_primitive = std::array{true, true, true, false, false, false};
        const auto tags         = std::array{TypeExpressionTag::EXPLICIT,
                                     TypeExpressionTag::EXPLICIT,
                                     TypeExpressionTag::EXPLICIT,
                                     TypeExpressionTag::IMPLICIT,
                                     TypeExpressionTag::EXPLICIT,
                                     TypeExpressionTag::EXPLICIT};
        std::array<ExplicitTypeTag, std::size(tags)> explicit_tags;
        explicit_tags.fill(ExplicitTypeTag::EXPLICIT_IDENT);

        const auto* ast = pf.ast();
        const auto  expected_type_names =
            std::to_array<std::string>({"int", "uint", "bool", {}, "LongNum", "Conch"});
        REQUIRE(ast->statements.length == std::size(expected_identifiers));

        Statement* stmt;
        for (size_t i = 0; i < std::size(expected_identifiers); i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));
            test_decl_statement(stmt,
                                is_const[i],
                                expected_identifiers[i],
                                is_nullable[i],
                                is_primitive[i],
                                tags[i],
                                explicit_tags[i],
                                expected_type_names[i]);
        }
    }

    SECTION("Primitive alias type decls") {
        const auto inputs = std::array{
            "type a = int",
            "type a = uint",
            "type a = float",
            "type a = string",
            "type a = byte",
            "type a = bool",
            "type a = void",
        };

        const auto expected_primitives = std::to_array<std::pair<std::string, TokenType>>({
            {"int", TokenType::INT_TYPE},
            {"uint", TokenType::UINT_TYPE},
            {"float", TokenType::FLOAT_TYPE},
            {"string", TokenType::STRING_TYPE},
            {"byte", TokenType::BYTE_TYPE},
            {"bool", TokenType::BOOL_TYPE},
            {"void", TokenType::VOID_TYPE},
        });

        REQUIRE(inputs.size() == expected_primitives.size());
        for (size_t i = 0; i < inputs.size(); i++) {
            const char* input    = inputs[i];
            const auto& expected = expected_primitives[i];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* type_decl_stmt = reinterpret_cast<const TypeDeclStatement*>(stmt);
            test_identifier_expression(reinterpret_cast<const Expression*>(type_decl_stmt->ident),
                                       "a");
            REQUIRE(type_decl_stmt->primitive_alias);

            test_identifier_expression(type_decl_stmt->value, expected.first, expected.second);
        }
    }

    SECTION("Nullable type decl") {
        const char*         input = "type N = ?int";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* type_decl_stmt = reinterpret_cast<const TypeDeclStatement*>(stmt);
        test_identifier_expression(reinterpret_cast<const Expression*>(type_decl_stmt->ident), "N");
        REQUIRE_FALSE(type_decl_stmt->primitive_alias);

        test_type_expression(type_decl_stmt->value,
                             true,
                             true,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_IDENT,
                             "int");
    }
}

TEST_CASE("Jump statements") {
    SECTION("Standard jumps") {
        const char*         input = "return;\n"
                                    "return 5;\n"
                                    "break;"
                                    "break 10;\n"
                                    "return 993322;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 5);

        const auto values =
            std::to_array<std::optional<int64_t>>({std::nullopt, 5, std::nullopt, 10, 993322});

        Statement* stmt;
        for (size_t i = 0; i < ast->statements.length; i++) {
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));
            const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
            if (values[i]) { test_number_expression<int64_t>(jump_stmt->value, values[i].value()); }
        }
    }

    SECTION("Returns w/o sentinel semicolon") {
        const char*         input = "return 5";
        const ParserFixture pf{input};
        pf.check_errors();
        const auto* ast = pf.ast();

        REQUIRE(ast->statements.length == 1);
        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
        test_number_expression<int64_t>(jump_stmt->value, 5);
    }

    SECTION("Breaks w/o sentinel semicolon") {
        const char*         input = "break -5";
        const ParserFixture pf{input};
        pf.check_errors();
        const auto* ast = pf.ast();

        REQUIRE(ast->statements.length == 1);
        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
        const auto* prefix    = reinterpret_cast<const PrefixExpression*>(jump_stmt->value);
        test_number_expression<int64_t>(prefix->rhs, 5);
    }

    SECTION("Continue jumps") {
        SECTION("Correctly without value") {
            const char*         input = "continue";
            const ParserFixture pf{input};
            pf.check_errors();
            const auto* ast = pf.ast();

            REQUIRE(ast->statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
            REQUIRE_FALSE(jump_stmt->value);
        }

        SECTION("Incorrectly with value") {
            const char*         input = "continue 2";
            const ParserFixture pf{input};
            pf.check_errors();
            const auto* ast = pf.ast();

            REQUIRE(ast->statements.length == 2);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
            REQUIRE_FALSE(jump_stmt->value);

            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 1, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 2);
        }
    }
}

TEST_CASE("Identifier Expressions") {
    const char*         input = "foobar;";
    const ParserFixture pf{input};
    pf.check_errors();

    const auto* ast = pf.ast();
    REQUIRE(ast->statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));

    const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
    test_identifier_expression(expr_stmt->expression, "foobar");
}

TEST_CASE("Index expression") {
    const char*         input = "foo[bar]";
    const ParserFixture pf{input};
    pf.check_errors();

    const auto* ast = pf.ast();
    REQUIRE(ast->statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));

    const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
    const auto* index     = reinterpret_cast<const IndexExpression*>(expr_stmt->expression);
    test_identifier_expression(index->array, "foo");
    test_identifier_expression(index->idx, "bar");
}

TEST_CASE("Number-based expressions") {
    SECTION("Signed integer bases") {
        test_number_expression<int64_t>("5;", 5);
        test_number_expression<int64_t>("0b10011101101;", 0b10011101101);
        test_number_expression<int64_t>("0o1234567;", 342391);
        test_number_expression<int64_t>("0xFF8a91d;", 0xFF8a91d);
    }

    SECTION("Signed integer overflow") {
        const char*         input = "0xFFFFFFFFFFFFFFFF";
        const ParserFixture pf{input};
        const auto errors = std::to_array<std::string>({"SIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
        pf.check_errors(errors);
    }

    SECTION("Unsigned integer bases") {
        test_number_expression<uint64_t>("5u;", 5);
        test_number_expression<uint64_t>("0b10011101101u;", 0b10011101101);
        test_number_expression<uint64_t>("0o1234567U;", 342391);
        test_number_expression<uint64_t>("0xFF8a91du;", 0xFF8a91d);

        test_number_expression<uint64_t>("0xFFFFFFFFFFFFFFFFu;", 0xFFFFFFFFFFFFFFFF);
    }

    SECTION("Bytes") {
        const auto test_byte_expression = [](const char* input, uint8_t expected) -> void {
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* byte =
                reinterpret_cast<const ByteLiteralExpression*>(expr_stmt->expression);

            REQUIRE(byte->value == expected);
        };

        test_byte_expression("'3'", '3');
        test_byte_expression("'\\0'", '\0');
    }

    SECTION("Unsigned integer overflow") {
        const char*         input = "0x10000000000000000u";
        const ParserFixture pf{input};
        const auto errors = std::to_array<std::string>({"UNSIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
        pf.check_errors(errors);
    }

    SECTION("Floating points") {
        test_number_expression<double>("1023.0;", 1023.0);
        test_number_expression<double>("1023.234612;", 1023.234612);
        test_number_expression<double>("1023.234612e234;", 1023.234612e234);
    }
}

TEST_CASE("Basic prefix / infix expressions") {
    SECTION("Simple prefix expressions") {
        const auto cases = std::to_array<
            std::tuple<const char*, TokenType, std::variant<int64_t, uint64_t, double>>>({
            {"!5", TokenType::BANG, 5LL},
            {"-15u", TokenType::MINUS, 15ULL},
            {"!3.4", TokenType::BANG, 3.4},
            {"~0b101101", TokenType::NOT, 0b101101LL},
            {"!1.2345e100", TokenType::BANG, 1.2345e100},
        });

        for (const auto& [input, op, value] : cases) {
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* expr = reinterpret_cast<const PrefixExpression*>(expr_stmt->expression);

            REQUIRE(NODE_TOKEN(expr).type == op);
            if (std::holds_alternative<int64_t>(value)) {
                test_number_expression<int64_t>(expr->rhs, std::get<int64_t>(value));
            } else if (std::holds_alternative<uint64_t>(value)) {
                test_number_expression<uint64_t>(expr->rhs, std::get<uint64_t>(value));
            } else if (std::holds_alternative<double>(value)) {
                test_number_expression<double>(expr->rhs, std::get<double>(value));
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

        const auto standard_cases = std::to_array<TestCase>({
            {.input = "5 + 5;", .lvalue = 5LL, .op = TokenType::PLUS, .rvalue = 5LL},
            {.input = "5 - 5;", .lvalue = 5LL, .op = TokenType::MINUS, .rvalue = 5LL},
            {.input = "5.0 * 5.2;", .lvalue = 5.0, .op = TokenType::STAR, .rvalue = 5.2},
            {.input = "5.0 ** 5.2;", .lvalue = 5.0, .op = TokenType::STAR_STAR, .rvalue = 5.2},
            {.input = "4.9e2 / 5.1e3;", .lvalue = 4.9e2, .op = TokenType::SLASH, .rvalue = 5.1e3},
            {.input = "0x231 % 0xF;", .lvalue = 0x231, .op = TokenType::PERCENT, .rvalue = 0xF},
            {.input = "5 < 5;", .lvalue = 5LL, .op = TokenType::LT, .rvalue = 5LL},
            {.input = "5 <= 5;", .lvalue = 5LL, .op = TokenType::LTEQ, .rvalue = 5LL},
            {.input = "5 > 5;", .lvalue = 5LL, .op = TokenType::GT, .rvalue = 5LL},
            {.input = "5 >= 5;", .lvalue = 5LL, .op = TokenType::GTEQ, .rvalue = 5LL},
            {.input = "5 == 5;", .lvalue = 5LL, .op = TokenType::EQ, .rvalue = 5LL},
            {.input = "5 != 5;", .lvalue = 5LL, .op = TokenType::NEQ, .rvalue = 5LL},
            {.input  = "0b10111u & 0b10110u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::AND,
             .rvalue = 0b10110ULL},
            {.input  = "0b10111u | 0b10110u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::OR,
             .rvalue = 0b10110ULL},
            {.input  = "0b10111u ^ 0b10110u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::XOR,
             .rvalue = 0b10110ULL},
            {.input  = "0b10111u >> 5u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::SHR,
             .rvalue = 5ULL},
            {.input  = "0b10111u << 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::SHL,
             .rvalue = 4ULL},
            {.input  = "0b10111u..4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::DOT_DOT,
             .rvalue = 4ULL},
            {.input  = "0b10111u..=4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::DOT_DOT_EQ,
             .rvalue = 4ULL},
            {.input = "0b10111u is 4u;", .lvalue = 0b10111ULL, .op = TokenType::IS, .rvalue = 4ULL},
            {.input = "0b10111u in 4u;", .lvalue = 0b10111ULL, .op = TokenType::IN, .rvalue = 4ULL},

            {.input  = "0b10111u and 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::BOOLEAN_AND,
             .rvalue = 4ULL},
            {.input  = "0b10111u or 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::BOOLEAN_OR,
             .rvalue = 4ULL},

            {.input  = "0b10111u orelse 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::ORELSE,
             .rvalue = 4ULL},
        });

        const auto assignment_cases = std::to_array<TestCase>({
            {.input  = "0b10111u = 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u += 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::PLUS_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u -= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::MINUS_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u *= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::STAR_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u /= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::SLASH_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u %= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::PERCENT_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u &= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::AND_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u |= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::OR_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u <<= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::SHL_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u >>= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::SHR_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u ~= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::NOT_ASSIGN,
             .rvalue = 4ULL},
            {.input  = "0b10111u ^= 4u;",
             .lvalue = 0b10111ULL,
             .op     = TokenType::XOR_ASSIGN,
             .rvalue = 4ULL},
        });

        const auto test_infix = []<typename T>(const TestCase& t) -> void {
            const ParserFixture pf{t.input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const T*    expr      = reinterpret_cast<T*>(expr_stmt->expression);

            const auto pairs = std::array{
                std::pair{t.lvalue, expr->lhs},
                std::pair{t.rvalue, expr->rhs},
            };

            if constexpr (requires { expr->op; }) { REQUIRE(expr->op == t.op); }

            for (const auto& p : pairs) {
                if (std::holds_alternative<int64_t>(p.first)) {
                    test_number_expression<int64_t>(p.second, std::get<int64_t>(p.first));
                } else if (std::holds_alternative<uint64_t>(p.first)) {
                    test_number_expression<uint64_t>(p.second, std::get<uint64_t>(p.first));
                } else if (std::holds_alternative<double>(p.first)) {
                    test_number_expression<double>(p.second, std::get<double>(p.first));
                } else {
                    REQUIRE(false);
                }
            }
        };

        // Yay templated lambdas in C++ 20 so fun
        for (const auto& t : standard_cases) {
            test_infix.template operator()<InfixExpression>(t);
        }

        for (const auto& t : assignment_cases) {
            test_infix.template operator()<AssignmentExpression>(t);
        }
    }

    SECTION("Namespace expressions") {
        const ParserFixture pf{"Outer::inner"};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* namespace_expr =
            reinterpret_cast<const NamespaceExpression*>(expr_stmt->expression);

        test_identifier_expression(namespace_expr->outer, "Outer");
        test_identifier_expression(reinterpret_cast<const Expression*>(namespace_expr->inner),
                                   "inner");
    }

    SECTION("Operator precedence parsing") {
        const auto cases = std::to_array<std::pair<const char*, std::string>>({
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
            {"5 * 5 ** 2", "(5 * (5 ** 2))"},
            {"2 / (5 + 5)", "(2 / (5 + 5))"},
            {"-(5 + 5)", "(-(5 + 5))"},
            {"!(true == true)", "(!(true == true))"},
            {"a + add(b * c) + d", "((a + add((b * c))) + d)"},
            {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))",
             "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
            {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"},
        });

        for (const auto& [input, expected] : cases) {
            group_expressions = true;
            ParserFixture pf{input};
            pf.check_errors();

            SBFixture sb{expected.length()};
            REQUIRE(STATUS_OK(ast_reconstruct(pf.ast_mut(), sb.sb())));
            REQUIRE(expected == sb.to_string());
        }
    }

    SECTION("Malformed expressions") {
        const auto cases = std::to_array<std::pair<const char*, std::array<std::string, 1>>>({
            {"!", {"PREFIX_MISSING_OPERAND [Ln 1, Col 1]"}},
            {"3+", {"INFIX_MISSING_RHS [Ln 1, Col 2]"}},
            {"3+4-", {"INFIX_MISSING_RHS [Ln 1, Col 4]"}},
        });

        for (const auto& [input, errors] : cases) {
            const ParserFixture pf{input};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Typeof expressions") {
    SECTION("Correct type introspection") {
        const char*         input = "type a = ?typeof 4u";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* type_decl = reinterpret_cast<const TypeDeclStatement*>(stmt);
        test_identifier_expression(reinterpret_cast<const Expression*>(type_decl->ident), "a");
        REQUIRE_FALSE(type_decl->primitive_alias);

        test_type_expression(type_decl->value,
                             true,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_TYPEOF,
                             {});

        const auto* type = reinterpret_cast<const TypeExpression*>(type_decl->value);
        test_number_expression<uint64_t>(type->variant.explicit_type.variant.referred_type, 4);
    }

    SECTION("Malformed usages") {
        const auto cases = std::to_array<std::pair<const char*, std::vector<std::string>>>({
            {"typeof 1", {"No prefix parse function for TYPEOF found [Ln 1, Col 1]"}},
            {"const a: typeof 1 = 3", {"ILLEGAL_DECL_CONSTRUCT [Ln 1, Col 1]"}},
            {"var v: typeof g", {"ILLEGAL_DECL_CONSTRUCT [Ln 1, Col 1]"}},
            {"type a = typeof enum { a, }",
             {"REDUNDANT_TYPE_INTROSPECTION [Ln 1, Col 17]", "MALFORMED_TYPE_DECL [Ln 1, Col 1]"}},
            {"type a = typeof fn(): int",
             {"REDUNDANT_TYPE_INTROSPECTION [Ln 1, Col 17]",
              "MALFORMED_TYPE_DECL [Ln 1, Col 1]",
              "Expected token LBRACE, found END [Ln 1, Col 26]"}},
            {"type a = typeof struct { a: int, }",
             {"REDUNDANT_TYPE_INTROSPECTION [Ln 1, Col 17]", "MALFORMED_TYPE_DECL [Ln 1, Col 1]"}},
            {"type a = typeof ?enum {a, }",
             {"No prefix parse function for WHAT found [Ln 1, Col 17]",
              "MALFORMED_TYPE_DECL [Ln 1, Col 1]"}},
        });

        for (const auto& [input, errors] : cases) {
            const ParserFixture pf{input};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Bool expressions") {
    SECTION("Basic expressions") {
        const char*         input = "true;\n"
                                    "false;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 2);

        const auto expected_values = std::array{true, false};

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));

            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_bool_expression(expr_stmt->expression, expected_values[i]);
        }
    }
}

TEST_CASE("String expressions") {
    SECTION("Single-line strings") {
        const char*         input = R"("This is a string";
                              "Hello, 'World'!";
                              "";)";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 3);

        const auto expected_strings =
            std::to_array<std::string>({"This is a string", "Hello, 'World'!", ""});
        const auto expected_literals = std::to_array<std::string>({
            R"("This is a string")",
            R"("Hello, 'World'!")",
            R"("")",
        });

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));

            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_string_expression(expr_stmt->expression, expected_strings[i]);
        }
    }

    SECTION("Multiline strings") {
        const char*         input = "\\\\This is a string\n"
                                    ";"
                                    "\\\\Hello, 'World'!\n"
                                    "\\\\\n"
                                    ";"
                                    "\\\\\n"
                                    ";";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 3);

        const auto expected_strings =
            std::to_array<std::string>({"This is a string", "Hello, 'World'!\n", ""});
        const auto expected_literals = std::to_array<std::string>({
            "This is a string",
            R"(Hello, 'World'!
            \\)",
            "",
        });

        for (size_t i = 0; i < ast->statements.length; i++) {
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, i, static_cast<void*>(&stmt))));

            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_string_expression(expr_stmt->expression, expected_strings[i]);
        }
    }
}

TEST_CASE("Conditional expressions") {
    SECTION("If without alternate") {
        const char*         input = "if (x < y) { x }";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);

        const auto* if_expr = reinterpret_cast<const IfExpression*>(expr_stmt->expression);
        REQUIRE_FALSE(if_expr->alternate);

        const auto* condition = reinterpret_cast<const InfixExpression*>(if_expr->condition);
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        const auto* consequence = reinterpret_cast<const BlockStatement*>(if_expr->consequence);
        REQUIRE(consequence->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_identifier_expression(expr_stmt->expression, "x");
    }

    SECTION("If with alternate") {
        const char*         input = "if (x < y) { x } else { y }";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);

        const auto* if_expr = reinterpret_cast<const IfExpression*>(expr_stmt->expression);
        REQUIRE(if_expr->alternate);

        const auto* condition = reinterpret_cast<const InfixExpression*>(if_expr->condition);
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        const auto* consequence = reinterpret_cast<const BlockStatement*>(if_expr->consequence);
        REQUIRE(consequence->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&consequence->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_identifier_expression(expr_stmt->expression, "x");

        const auto* alternate = reinterpret_cast<const BlockStatement*>(if_expr->alternate);
        REQUIRE(alternate->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&alternate->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_identifier_expression(expr_stmt->expression, "y");
    }

    SECTION("If/Else with non-block assignment") {
        const char*         input = "const val := if (x >= y) 1 else 2;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        test_decl_statement(stmt, true, "val");
        const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);
        REQUIRE(decl_stmt->type->tag == IMPLICIT);

        const auto* if_expr = reinterpret_cast<const IfExpression*>(decl_stmt->value);
        REQUIRE(if_expr->alternate);

        const auto* condition = reinterpret_cast<const InfixExpression*>(if_expr->condition);
        REQUIRE(condition->op == TokenType::GTEQ);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        const auto* consequence =
            reinterpret_cast<const ExpressionStatement*>(if_expr->consequence);
        test_number_expression<int64_t>(consequence->expression, 1);

        const auto* alternate = reinterpret_cast<const ExpressionStatement*>(if_expr->alternate);
        test_number_expression<int64_t>(alternate->expression, 2);
    }

    SECTION("If/Else with return statement") {
        const char*         input = "if (true) return 1; else return 2;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);

        const auto* if_expr = reinterpret_cast<const IfExpression*>(expr_stmt->expression);
        REQUIRE(if_expr->alternate);

        const auto* condition = reinterpret_cast<const BoolLiteralExpression*>(if_expr->condition);
        REQUIRE(condition->value);

        const auto* consequence = reinterpret_cast<const JumpStatement*>(if_expr->consequence);
        test_number_expression<int64_t>(consequence->value, 1);

        const auto* alternate = reinterpret_cast<const JumpStatement*>(if_expr->alternate);
        test_number_expression<int64_t>(alternate->value, 2);
    }

    SECTION("If/Else with nested ifs and terminal else") {
        const char* input = "if (x < y) {return -1;} else if (x > y) {return 1;} else {return 0;}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);

        // Verify the first condition
        const auto* if_expr = reinterpret_cast<const IfExpression*>(expr_stmt->expression);
        REQUIRE(if_expr->alternate);

        const auto* condition = reinterpret_cast<const InfixExpression*>(if_expr->condition);
        REQUIRE(condition->op == TokenType::LT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        // Verify the first consequence
        const auto* consequence_one = reinterpret_cast<const BlockStatement*>(if_expr->consequence);
        REQUIRE(consequence_one->statements.length == 1);
        REQUIRE(
            STATUS_OK(array_list_get(&consequence_one->statements, 0, static_cast<void*>(&stmt))));
        const auto* jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
        const auto* neg_num   = reinterpret_cast<const PrefixExpression*>(jump_stmt->value);
        REQUIRE(NODE_TOKEN(neg_num).type == TokenType::MINUS);
        test_number_expression<int64_t>(neg_num->rhs, 1);

        // Verify the first alternate and its condition
        const auto* alternate_one =
            reinterpret_cast<const ExpressionStatement*>(if_expr->alternate);
        if_expr = reinterpret_cast<const IfExpression*>(alternate_one->expression);
        REQUIRE(if_expr->alternate);

        condition = reinterpret_cast<const InfixExpression*>(if_expr->condition);
        REQUIRE(condition->op == TokenType::GT);
        test_identifier_expression(condition->lhs, "x");
        test_identifier_expression(condition->rhs, "y");

        // Verify the second consequence
        const auto* consequence_two = reinterpret_cast<const BlockStatement*>(if_expr->consequence);
        REQUIRE(consequence_two->statements.length == 1);
        REQUIRE(
            STATUS_OK(array_list_get(&consequence_two->statements, 0, static_cast<void*>(&stmt))));
        jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
        test_number_expression<int64_t>(jump_stmt->value, 1);

        // Verify the second alternate
        const auto* alternate_two = reinterpret_cast<const BlockStatement*>(if_expr->alternate);
        REQUIRE(alternate_two->statements.length == 1);
        REQUIRE(
            STATUS_OK(array_list_get(&alternate_two->statements, 0, static_cast<void*>(&stmt))));
        jump_stmt = reinterpret_cast<const JumpStatement*>(stmt);
        test_number_expression<int64_t>(jump_stmt->value, 0);
    }
}

TEST_CASE("Function literals") {
    struct TestParameter {
        std::string name;

        bool              is_ref    = false;
        bool              nullable  = false;
        bool              primitive = true;
        TypeExpressionTag tag       = TypeExpressionTag::EXPLICIT;
        std::string       type_name = "int";

        std::optional<int64_t> default_value = std::nullopt;
    };

    const auto test_parameters = [](const ArrayList*               actuals,
                                    std::span<const TestParameter> expecteds) -> void {
        REQUIRE(actuals->length == expecteds.size());
        for (size_t i = 0; i < actuals->length; i++) {
            Parameter parameter;
            REQUIRE(STATUS_OK(array_list_get(actuals, i, &parameter)));
            const auto& expected_parameter = expecteds[i];

            REQUIRE(parameter.is_ref == expected_parameter.is_ref);
            test_identifier_expression(reinterpret_cast<const Expression*>(parameter.ident),
                                       expected_parameter.name);
            test_type_expression(reinterpret_cast<const Expression*>(parameter.type),
                                 expected_parameter.nullable,
                                 expected_parameter.primitive,
                                 expected_parameter.tag,
                                 ExplicitTypeTag::EXPLICIT_IDENT,
                                 expected_parameter.type_name);

            if (expected_parameter.default_value) {
                test_number_expression<int64_t>(parameter.default_value,
                                                expected_parameter.default_value.value());
            } else {
                REQUIRE_FALSE(parameter.default_value);
                REQUIRE_FALSE(expected_parameter.default_value);
            }
        }
    };

    SECTION("Functions as types") {
        SECTION("Correct declaration") {
            const auto expected_params =
                std::array{TestParameter{.name = "a", .is_ref = true}, TestParameter{.name = "b"}};

            const char*         input = "var add: fn(ref a: int, b: int): int;";
            const ParserFixture pf{input};
            pf.check_errors();
            const auto* ast = pf.ast();

            REQUIRE(ast->statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));

            test_decl_statement(stmt,
                                false,
                                "add",
                                false,
                                false,
                                TypeExpressionTag::EXPLICIT,
                                ExplicitTypeTag::EXPLICIT_FN,
                                {});
            const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);
            REQUIRE_FALSE(decl_stmt->value);

            const auto* type_expr     = decl_stmt->type;
            const auto  explicit_type = type_expr->variant.explicit_type;
            const auto  function_type = explicit_type.variant.function_type;
            test_parameters(&function_type.parameters, expected_params);
            test_type_expression(reinterpret_cast<const Expression*>(function_type.return_type),
                                 false,
                                 true,
                                 TypeExpressionTag::EXPLICIT,
                                 ExplicitTypeTag::EXPLICIT_IDENT,
                                 "int");
        }

        SECTION("Error cases") {
            std::array<std::string, 1> errors;
            SECTION("Incorrect declaration with first default") {
                const char*         input = "var add: fn(a: int = 1, b: int): ?int;";
                const ParserFixture pf{input};
                errors = {"MALFORMED_FUNCTION_LITERAL [Ln 1, Col 8]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect declaration with second default") {
                const char*         input = "var add: fn(a: int, b: int = 2): int;";
                const ParserFixture pf{input};
                errors = {"MALFORMED_FUNCTION_LITERAL [Ln 1, Col 8]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect declaration with missing return type") {
                const char*         input = "var add: fn(a: int, b: int):;";
                const ParserFixture pf{input};
                errors = {"MALFORMED_FUNCTION_LITERAL [Ln 1, Col 28]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect declaration with both default") {
                const char*         input = "const add: fn(a: int = -345, b: uint = 209u);";
                const ParserFixture pf{input};
                errors = {"Expected token COLON, found SEMICOLON [Ln 1, Col 45]"};
                pf.check_errors(errors);
            }
        }
    }

    SECTION("Parameter allocation") {
        struct TestReturnType {
            std::string type_name;
            bool        nullable;
            bool        primitive;
        };

        const auto cases =
            std::to_array<std::tuple<const char*, std::vector<TestParameter>, TestReturnType>>({
                {"fn(): void {};", {}, {.type_name = "void", .nullable = false, .primitive = true}},
                {"fn(x: int): Blk {};",
                 {TestParameter{.name = "x"}},
                 {.type_name = "Blk", .nullable = false, .primitive = false}},
                {"fn(x: int, y: int, z: int): int {};",
                 {TestParameter{.name = "x"},
                  TestParameter{.name = "y"},
                  TestParameter{.name = "z"}},
                 {.type_name = "int", .nullable = false, .primitive = true}},
                {"fn(x: int, ref y: int = 2, z: int): ?Sock {};",
                 {TestParameter{.name = "x"},
                  TestParameter{.name = "y", .is_ref = true, .default_value = 2},
                  TestParameter{.name = "z"}},
                 {.type_name = "Sock", .nullable = true, .primitive = false}},
                {"fn(x: int, y: int, z: int = 3): ?uint {};",
                 {TestParameter{.name = "x"},
                  TestParameter{.name = "y"},
                  TestParameter{.name = "z", .default_value = 3}},
                 {.type_name = "uint", .nullable = true, .primitive = true}},
            });

        for (const auto& [input, expected_params, expected_return] : cases) {
            const ParserFixture pf{input};
            pf.check_errors();
            const auto* ast = pf.ast();

            REQUIRE(ast->statements.length == 1);
            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));

            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* function =
                reinterpret_cast<const FunctionExpression*>(expr_stmt->expression);
            REQUIRE(function->body->statements.length == 0);

            const auto* parameters = &function->parameters;
            test_parameters(parameters, expected_params);

            test_type_expression(reinterpret_cast<const Expression*>(function->return_type),
                                 expected_return.nullable,
                                 expected_return.primitive,
                                 TypeExpressionTag::EXPLICIT,
                                 ExplicitTypeTag::EXPLICIT_IDENT,
                                 expected_return.type_name);
        }

        SECTION("Implicit parameter type") {
            const char*         input = "fn(a := 2): int";
            const ParserFixture pf{input};
            const auto          errors = std::to_array<std::string>(
                {"IMPLICIT_FN_PARAM_TYPE [Ln 1, Col 9]",
                          "No prefix parse function for RPAREN found [Ln 1, Col 10]",
                          "No prefix parse function for COLON found [Ln 1, Col 11]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 13]"});
            pf.check_errors(errors);
        }
    }

    SECTION("Call expression with identifier function") {
        const char*         input = "add(1, 2 * 3, ref w, 4 + 5);";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* call      = reinterpret_cast<const CallExpression*>(expr_stmt->expression);

        // Verify function identifier
        const auto* function = reinterpret_cast<const IdentifierExpression*>(call->function);
        std::string expected_function_name = "add";
        REQUIRE(expected_function_name == function->name.ptr);

        const ArrayList* arguments = &call->arguments;
        REQUIRE(arguments->length == 4);

        // Verify arguments
        CallArgument first;
        REQUIRE(STATUS_OK(array_list_get(arguments, 0, &first)));
        REQUIRE_FALSE(first.is_ref);
        test_number_expression<int64_t>(first.argument, 1);

        CallArgument second;
        REQUIRE(STATUS_OK(array_list_get(arguments, 1, &second)));
        REQUIRE_FALSE(second.is_ref);
        const auto* argument = reinterpret_cast<const InfixExpression*>(second.argument);
        test_number_expression<int64_t>(argument->lhs, 2);
        REQUIRE(argument->op == TokenType::STAR);
        test_number_expression<int64_t>(argument->rhs, 3);

        CallArgument third;
        REQUIRE(STATUS_OK(array_list_get(arguments, 2, &third)));
        REQUIRE(third.is_ref);
        test_identifier_expression(third.argument, "w");

        CallArgument fourth;
        REQUIRE(STATUS_OK(array_list_get(arguments, 3, &fourth)));
        REQUIRE_FALSE(fourth.is_ref);
        argument = reinterpret_cast<const InfixExpression*>(fourth.argument);
        test_number_expression<int64_t>(argument->lhs, 4);
        REQUIRE(argument->op == TokenType::PLUS);
        test_number_expression<int64_t>(argument->rhs, 5);
    }
}

TEST_CASE("Enum declarations") {
    struct EnumVariantTestCase {
        std::string            expected_name;
        std::optional<int64_t> expected_value = std::nullopt;
    };

    const auto test_enum_variants =
        [](const ArrayList*                     variants,
           std::span<const EnumVariantTestCase> expected_variants) -> void {
        REQUIRE(variants->length == expected_variants.size());
        for (size_t i = 0; i < variants->length; i++) {
            EnumVariant variant;
            REQUIRE(STATUS_OK(array_list_get(variants, i, &variant)));
            REQUIRE(variant.name);

            const auto& expected = expected_variants[i];
            REQUIRE(expected.expected_name == variant.name->name.ptr);

            if (expected.expected_value) {
                test_number_expression<int64_t>(variant.value, expected.expected_value.value());
            } else {
                REQUIRE_FALSE(variant.value);
            }
        }
    };

    SECTION("Correctly formed enums") {
        const auto inputs = std::to_array<const char*>({
            "enum { RED, BLUE, GREEN, }",
            "enum { RED, BLUE = 1, GREEN, }",
            "enum { RED = 100, BLUE = 20, GREEN = 3, }",
        });

        const auto expected_variants_lists = std::to_array<std::vector<EnumVariantTestCase>>({
            {{.expected_name = "RED"}, {.expected_name = "BLUE"}, {.expected_name = "GREEN"}},
            {{.expected_name = "RED"},
             {.expected_name = "BLUE", .expected_value = 1},
             {.expected_name = "GREEN"}},
            {{.expected_name = "RED", .expected_value = 100},
             {.expected_name = "BLUE", .expected_value = 20},
             {.expected_name = "GREEN", .expected_value = 3}},
        });

        REQUIRE(inputs.size() == expected_variants_lists.size());
        for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
            const char* input             = inputs[test_idx];
            const auto& expected_variants = expected_variants_lists[test_idx];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* enum_expr = reinterpret_cast<const EnumExpression*>(expr_stmt->expression);

            test_enum_variants(&enum_expr->variants, expected_variants);
        }
    }

    SECTION("Enums as types") {
        const char* input = "var a: enum { RED, BLUE = 100, GREEN, };";
        const auto  expected_variants_list =
            std::to_array<EnumVariantTestCase>({{.expected_name = "RED"},
                                                {.expected_name = "BLUE", .expected_value = 100},
                                                {.expected_name = "GREEN"}});

        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        test_decl_statement(stmt, false, "a");
        const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);

        const auto* type_expr = decl_stmt->type;
        test_type_expression(reinterpret_cast<const Expression*>(type_expr),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_ENUM,
                             {});

        const auto* enum_type = type_expr->variant.explicit_type.variant.enum_type;
        test_enum_variants(&enum_type->variants, expected_variants_list);
    }

    SECTION("Enums in type decls") {
        const char* input = "type Colors = enum { RED, BLUE = 100, GREEN, };";
        const auto  expected_variants_list =
            std::to_array<EnumVariantTestCase>({{.expected_name = "RED"},
                                                {.expected_name = "BLUE", .expected_value = 100},
                                                {.expected_name = "GREEN"}});

        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* type_decl_stmt = reinterpret_cast<const TypeDeclStatement*>(stmt);
        test_identifier_expression(reinterpret_cast<const Expression*>(type_decl_stmt->ident),
                                   "Colors");
        REQUIRE_FALSE(type_decl_stmt->primitive_alias);

        const auto* type_expr = reinterpret_cast<const TypeExpression*>(type_decl_stmt->value);
        test_type_expression(reinterpret_cast<const Expression*>(type_expr),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_ENUM,
                             {});

        const auto* enum_type = type_expr->variant.explicit_type.variant.enum_type;
        test_enum_variants(&enum_type->variants, expected_variants_list);
    }

    SECTION("Malformed enum expressions") {
        std::vector<std::string> errors;
        SECTION("Empty enum body") {
            const char*         input = "enum {}";
            const ParserFixture pf{input};
            errors = {"ENUM_MISSING_VARIANTS [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Missing trailing comma") {
            const char*         input = "enum { a, b, c }";
            const ParserFixture pf{input};
            errors = {"Expected token COMMA, found RBRACE [Ln 1, Col 16]",
                      "MISSING_TRAILING_COMMA [Ln 1, Col 14]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 16]"};
            pf.check_errors(errors);
        }

        SECTION("Missing internal comma") {
            const char*         input = "enum { a, b c, }";
            const ParserFixture pf{input};
            errors = {"Expected token COMMA, found IDENT [Ln 1, Col 13]",
                      "MISSING_TRAILING_COMMA [Ln 1, Col 11]",
                      "No prefix parse function for COMMA found [Ln 1, Col 14]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 16]"};
            pf.check_errors(errors);
        }

        SECTION("All commas omitted") {
            const char*         input = "enum { a b c }";
            const ParserFixture pf{input};
            errors = {"Expected token COMMA, found IDENT [Ln 1, Col 10]",
                      "MISSING_TRAILING_COMMA [Ln 1, Col 8]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 14]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Struct declarations") {
    struct StructMemberTestCase {
        std::string            member_name;
        std::string            type_name;
        bool                   nullable      = false;
        bool                   primitive     = true;
        std::optional<int64_t> default_value = std::nullopt;
    };

    const auto test_struct_members =
        [](const ArrayList*                      members,
           std::span<const StructMemberTestCase> expected_members) -> void {
        REQUIRE(members->length == expected_members.size());
        for (size_t i = 0; i < members->length; i++) {
            StructMember member;
            REQUIRE(STATUS_OK(array_list_get(members, i, &member)));
            REQUIRE(member.name);
            REQUIRE(member.type);

            const auto& expected = expected_members[i];
            test_identifier_expression(reinterpret_cast<const Expression*>(member.name),
                                       expected.member_name);
            test_type_expression(reinterpret_cast<const Expression*>(member.type),
                                 expected.nullable,
                                 expected.primitive,
                                 TypeExpressionTag::EXPLICIT,
                                 ExplicitTypeTag::EXPLICIT_IDENT,
                                 expected.type_name);

            if (expected.default_value) {
                REQUIRE(member.default_value);
                test_number_expression<int64_t>(member.default_value,
                                                expected.default_value.value());
            } else {
                REQUIRE_FALSE(member.default_value);
            }
        }
    };

    SECTION("Correctly formed structs") {
        const auto inputs = std::array{
            "struct { a: int, }",
            "struct { a: int, b: uint, c: ?Woah, d: int = 1, }",
        };

        const auto expected_member_lists = std::to_array<std::vector<StructMemberTestCase>>({
            {{.member_name = "a", .type_name = "int"}},
            {{.member_name = "a", .type_name = "int"},
             {.member_name = "b", .type_name = "uint"},
             {.member_name = "c", .type_name = "Woah", .nullable = true, .primitive = false},
             {.member_name   = "d",
              .type_name     = "int",
              .nullable      = false,
              .primitive     = true,
              .default_value = 1}},
        });

        REQUIRE(inputs.size() == expected_member_lists.size());
        for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
            const char* input            = inputs[test_idx];
            const auto& expected_members = expected_member_lists[test_idx];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* struct_expr =
                reinterpret_cast<const StructExpression*>(expr_stmt->expression);
            test_struct_members(&struct_expr->members, expected_members);
        }
    }

    SECTION("Structs as types") {
        const char* input                = "var a: struct { a: int, b: ?uint, };";
        const auto  expected_member_list = std::to_array<StructMemberTestCase>(
            {{.member_name = "a", .type_name = "int"},
              {.member_name = "b", .type_name = "uint", .nullable = true}});

        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        test_decl_statement(stmt, false, "a");
        const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);

        const auto* type_expr = decl_stmt->type;
        test_type_expression(reinterpret_cast<const Expression*>(type_expr),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_STRUCT,
                             {});

        const auto* struct_type = type_expr->variant.explicit_type.variant.struct_type;
        test_struct_members(&struct_type->members, expected_member_list);
    }

    SECTION("Malformed struct expressions") {
        std::vector<std::string> errors;
        SECTION("Empty struct body") {
            const char*         input = "struct {}";
            const ParserFixture pf{input};
            errors = {"STRUCT_MISSING_MEMBERS [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Missing trailing comma") {
            const char*         input = "struct { a: int, b: int }";
            const ParserFixture pf{input};
            errors = {"MISSING_TRAILING_COMMA [Ln 1, Col 25]"};
            pf.check_errors(errors);
        }

        SECTION("Missing internal comma") {
            const char*         input = "struct { a: int, b: int c: int, }";
            const ParserFixture pf{input};
            errors = {"MISSING_TRAILING_COMMA [Ln 1, Col 25]",
                      "No prefix parse function for COLON found [Ln 1, Col 26]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 28]",
                      "No prefix parse function for COMMA found [Ln 1, Col 31]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 33]"};
            pf.check_errors(errors);
        }

        SECTION("All commas omitted") {
            const char*         input = "struct { a: int b: int c: int }";
            const ParserFixture pf{input};
            errors = {"MISSING_TRAILING_COMMA [Ln 1, Col 17]",
                      "No prefix parse function for COLON found [Ln 1, Col 18]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 20]",
                      "No prefix parse function for COLON found [Ln 1, Col 25]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 27]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 31]"};
            pf.check_errors(errors);
        }

        SECTION("Erroneous declaration") {
            const char*         input = "struct { const a: int = 1; }";
            const ParserFixture pf{input};
            errors = {"Expected token IDENT, found CONST [Ln 1, Col 10]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 28]"};
            pf.check_errors(errors);
        }

        SECTION("Implicit member type declaration") {
            const char*         input = "struct { a := 1, }";
            const ParserFixture pf{input};
            errors = {"STRUCT_MEMBER_NOT_EXPLICIT [Ln 1, Col 15]",
                      "No prefix parse function for COMMA found [Ln 1, Col 16]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 18]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Nil expressions") {
    const char*         input = "nil";
    const ParserFixture pf{input};
    pf.check_errors();

    const auto* ast = pf.ast();
    REQUIRE(ast->statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
    const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
    REQUIRE(slice_equals_str_z(&NODE_TOKEN(expr_stmt->expression).slice, "nil"));
}

TEST_CASE("Impl statements") {
    SECTION("Correct block") {
        const char*         input = "impl Obj<T> { const a := 1; }";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* impl_stmt = reinterpret_cast<const ImplStatement*>(stmt);

        test_identifier_expression(reinterpret_cast<const Expression*>(impl_stmt->parent), "Obj");

        REQUIRE(impl_stmt->generics.length == 1);
        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&impl_stmt->generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");

        REQUIRE(impl_stmt->implementation->statements.length == 1);
        REQUIRE(STATUS_OK(
            array_list_get(&impl_stmt->implementation->statements, 0, static_cast<void*>(&stmt))));
        test_decl_statement(stmt,
                            true,
                            "a",
                            false,
                            false,
                            TypeExpressionTag::IMPLICIT,
                            ExplicitTypeTag::EXPLICIT_IDENT,
                            {});
    }

    SECTION("Malformed impl blocks") {
        std::array<std::string, 1> errors;
        SECTION("Missing ident") {
            const char*         input = "impl { const a := 1; }";
            const ParserFixture pf{input};
            errors = {"Expected token IDENT, found LBRACE [Ln 1, Col 6]"};
            pf.check_errors(errors);
        }

        SECTION("Empty implementation") {
            const char*         input = "impl Obj {}";
            const ParserFixture pf{input};
            errors = {"EMPTY_IMPL_BLOCK [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Empty generics") {
            const char*         input = "impl Obj<> { const a := 1; }";
            const ParserFixture pf{input};
            errors = {"EMPTY_GENERIC_LIST [Ln 1, Col 10]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Import Statements") {
    struct ImportTestCase {
        ImportTag                  expected_tag;
        std::string                expected_literal;
        std::optional<std::string> expected_alias = std::nullopt;
    };

    SECTION("Correct imports") {
        const auto inputs = std::array{
            "import std",
            "import array;",
            R"(import "util/test.cch" as test)",
            "import hash as Hash",
        };

        const auto expected_imports = std::to_array<ImportTestCase>({
            {.expected_tag = ImportTag::STANDARD, .expected_literal = "std"},
            {.expected_tag = ImportTag::STANDARD, .expected_literal = "array"},
            {.expected_tag     = ImportTag::USER,
             .expected_literal = "util/test.cch",
             .expected_alias   = "test"},
            {.expected_tag     = ImportTag::STANDARD,
             .expected_literal = "hash",
             .expected_alias   = "Hash"},
        });

        REQUIRE(inputs.size() == expected_imports.size());
        for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
            const char* input    = inputs[test_idx];
            const auto& expected = expected_imports[test_idx];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* import_stmt = reinterpret_cast<const ImportStatement*>(stmt);

            REQUIRE(import_stmt->tag == expected.expected_tag);
            if (import_stmt->tag == ImportTag::STANDARD) {
                test_identifier_expression(
                    reinterpret_cast<const Expression*>(import_stmt->variant.standard_import),
                    expected.expected_literal);
            } else {
                test_string_expression(
                    reinterpret_cast<const Expression*>(import_stmt->variant.user_import),
                    expected.expected_literal);
            }

            if (import_stmt->alias != nullptr) {
                REQUIRE(expected.expected_alias);
                test_identifier_expression(reinterpret_cast<const Expression*>(import_stmt->alias),
                                           expected.expected_alias.value());
            } else {
                REQUIRE_FALSE(expected.expected_alias);
            }
        }
    }

    SECTION("Malformed import blocks") {
        std::array<std::string, 1> errors;
        SECTION("Incorrect token") {
            const char*         input = "import 1";
            const ParserFixture pf{input};
            errors = {"UNEXPECTED_TOKEN [Ln 1, Col 8]"};
            pf.check_errors(errors);
        }

        SECTION("User import without alias") {
            const char*         input = R"(import "some_file.cch")";
            const ParserFixture pf{input};
            errors = {"USER_IMPORT_MISSING_ALIAS [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Match expressions") {
    struct MatchArmTestCase {
        int64_t  lhs;
        uint64_t expected_return_value;
    };

    struct MatchTestCase {
        std::string                   expected_expr_name;
        std::vector<MatchArmTestCase> expected_arms;
        std::optional<int64_t>        otherwise = std::nullopt;
    };

    SECTION("Correct match") {
        const auto inputs = std::array{
            "match In { 1 => return 90u;, }",
            "match Out { 1 => return 90u;, 2 => return 0b1011u, };",
            "match Out { 1 => return 90u;, 2 => return 0b1011u, } else 5",
        };

        const auto expected_matches = std::to_array<MatchTestCase>({
            {.expected_expr_name = "In",
             .expected_arms      = {{.lhs = 1, .expected_return_value = 90ULL}}},
            {.expected_expr_name = "Out",
             .expected_arms      = {{.lhs = 1, .expected_return_value = 90ULL},
                                    {.lhs = 2, .expected_return_value = 0b1011ULL}}},
            {.expected_expr_name = "Out",
             .expected_arms      = {{.lhs = 1, .expected_return_value = 90ULL},
                                    {.lhs = 2, .expected_return_value = 0b1011ULL}},
             .otherwise          = 5},
        });

        REQUIRE(inputs.size() == expected_matches.size());
        for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
            const char* input    = inputs[test_idx];
            const auto& expected = expected_matches[test_idx];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* match_expr =
                reinterpret_cast<const MatchExpression*>(expr_stmt->expression);

            test_identifier_expression(match_expr->expression, expected.expected_expr_name);
            REQUIRE(match_expr->arms.length == expected.expected_arms.size());

            MatchArm arm;
            for (size_t arm_idx = 0; arm_idx < match_expr->arms.length; arm_idx++) {
                const auto& expected_arm = expected.expected_arms[arm_idx];

                REQUIRE(STATUS_OK(array_list_get(&match_expr->arms, arm_idx, &arm)));
                test_number_expression<int64_t>(arm.pattern, expected_arm.lhs);

                const auto* jump = reinterpret_cast<const JumpStatement*>(arm.dispatch);
                test_number_expression<uint64_t>(jump->value, expected_arm.expected_return_value);
            }

            if (match_expr->catch_all != nullptr) {
                REQUIRE(expected.otherwise);
                const auto* otherwise =
                    reinterpret_cast<const ExpressionStatement*>(match_expr->catch_all);
                test_number_expression<int64_t>(otherwise->expression, expected.otherwise.value());
            } else {
                REQUIRE_FALSE(expected.otherwise);
            }
        }
    }

    SECTION("Malformed match expressions") {
        std::vector<std::string> errors;
        SECTION("No arms") {
            const char*         input = "match Out { }";
            const ParserFixture pf{input};
            errors = {"ARMLESS_MATCH_EXPR [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Illegal catch-all") {
            const char*         input = "match Out { a => 4, } else const b := 4";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_MATCH_CATCH_ALL [Ln 1, Col 28]"};
            pf.check_errors(errors);
        }

        SECTION("Standard declarations in arm") {
            const char*         input = "match true { 1 => const a := 1, }";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_MATCH_ARM [Ln 1, Col 19]",
                      "No prefix parse function for WALRUS found [Ln 1, Col 27]",
                      "No prefix parse function for COMMA found [Ln 1, Col 31]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 33]"};
            pf.check_errors(errors);
        }

        SECTION("Type declarations in arm") {
            const char*         input = "match true { 1 => type a = Test, }";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_MATCH_ARM [Ln 1, Col 19]",
                      "No prefix parse function for COMMA found [Ln 1, Col 32]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 34]"};
            pf.check_errors(errors);
        }

        SECTION("Impl statements in arm") {
            const char*         input = "match true { 1 => impl Obj { const a := 1; }, }";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_MATCH_ARM [Ln 1, Col 19]",
                      "No prefix parse function for COMMA found [Ln 1, Col 45]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 47]"};
            pf.check_errors(errors);
        }

        SECTION("Import declarations in arm") {
            const char*         input = "match true { 1 => import std, }";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_MATCH_ARM [Ln 1, Col 19]",
                      "No prefix parse function for COMMA found [Ln 1, Col 29]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 31]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Array expressions") {
    struct ArrayTestCase {
        std::optional<uint64_t> expected_size;
        std::vector<int64_t>    expected_items;
    };

    SECTION("Correct int arrays") {
        const auto inputs = std::array{
            "[1uz]{1,}",
            "[0b11uz]{1, 2, 3, }",
            "[_]{1, 2, }",
        };

        const auto expected_arrays = std::to_array<ArrayTestCase>({
            {.expected_size = 1, .expected_items = {1}},
            {.expected_size = 0b11, .expected_items = {1, 2, 3}},
            {.expected_size = std::nullopt, .expected_items = {1, 2}},
        });

        REQUIRE(inputs.size() == expected_arrays.size());
        for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
            const char* input    = inputs[test_idx];
            const auto& expected = expected_arrays[test_idx];

            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* array_expr =
                reinterpret_cast<const ArrayLiteralExpression*>(expr_stmt->expression);

            if (array_expr->inferred_size) {
                REQUIRE_FALSE(expected.expected_size);
            } else {
                REQUIRE(array_expr->items.length == expected.expected_size.value());
            }
            REQUIRE(array_expr->items.length == expected.expected_items.size());

            Expression* item;
            for (size_t i = 0; i < array_expr->items.length; i++) {
                REQUIRE(
                    STATUS_OK(array_list_get(&array_expr->items, i, static_cast<void*>(&item))));
                test_number_expression(item, expected.expected_items[i]);
            }
        }
    }

    SECTION("Arrays as types") {
        struct ArrayTypeTestCase {
            std::string           decl_name;
            std::vector<uint64_t> expected_dims;
            bool                  array_nullable;
            bool                  inner_nullable;
        };

        SECTION("Correct array types") {
            const auto inputs = std::array{
                "var a: [1uz]int;",
                "var a: [1uz, 2uz]int;",
                "var a: ?[1uz, 2uz]int;",
                "var a: [1uz, 2uz]?int;",
                "var a: ?[1uz, 2uz]?int;",
            };

            const auto expected_dims = std::to_array<ArrayTypeTestCase>({
                {.decl_name      = "a",
                 .expected_dims  = {1},
                 .array_nullable = false,
                 .inner_nullable = false},
                {.decl_name      = "a",
                 .expected_dims  = {1, 2},
                 .array_nullable = false,
                 .inner_nullable = false},
                {.decl_name      = "a",
                 .expected_dims  = {1, 2},
                 .array_nullable = true,
                 .inner_nullable = false},
                {.decl_name      = "a",
                 .expected_dims  = {1, 2},
                 .array_nullable = false,
                 .inner_nullable = true},
                {.decl_name      = "a",
                 .expected_dims  = {1, 2},
                 .array_nullable = true,
                 .inner_nullable = true},
            });

            REQUIRE(inputs.size() == expected_dims.size());
            for (size_t test_idx = 0; test_idx < inputs.size(); test_idx++) {
                const char* input    = inputs[test_idx];
                const auto& expected = expected_dims[test_idx];

                const ParserFixture pf{input};
                pf.check_errors();

                const auto* ast = pf.ast();
                REQUIRE(ast->statements.length == 1);

                Statement* stmt;
                REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
                test_decl_statement(stmt, false, expected.decl_name);
                const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);

                const auto* decl_type = decl_stmt->type;
                REQUIRE(decl_type->tag == TypeExpressionTag::EXPLICIT);
                const auto explicit_type = decl_type->variant.explicit_type;
                REQUIRE(explicit_type.nullable == expected.array_nullable);
                REQUIRE_FALSE(explicit_type.primitive);
                REQUIRE(explicit_type.tag == ExplicitTypeTag::EXPLICIT_ARRAY);
                const auto explicit_array = explicit_type.variant.array_type;

                REQUIRE(explicit_array.dimensions.length == expected.expected_dims.size());
                for (size_t i = 0; i < explicit_array.dimensions.length; i++) {
                    uint64_t dim;
                    REQUIRE(STATUS_OK(array_list_get(&explicit_array.dimensions, i, &dim)));
                    REQUIRE(dim == expected.expected_dims[i]);
                }

                test_type_expression(reinterpret_cast<const Expression*>(explicit_array.inner_type),
                                     expected.inner_nullable,
                                     true,
                                     TypeExpressionTag::EXPLICIT,
                                     ExplicitTypeTag::EXPLICIT_IDENT,
                                     "int");
            }
        }

        SECTION("Malformed array types") {
            std::vector<std::string> errors;
            SECTION("Missing size token") {
                const char*         input = "var a: []int";
                const ParserFixture pf{input};
                errors = {"MISSING_ARRAY_SIZE_TOKEN [Ln 1, Col 5]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 10]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect token type") {
                const char*         input = R"(var a: ["wrong"]int)";
                const ParserFixture pf{input};
                errors = {"UNEXPECTED_ARRAY_SIZE_TOKEN [Ln 1, Col 9]",
                          "No prefix parse function for RBRACKET found [Ln 1, Col 16]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 17]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect token integer type") {
                const char*         input = "var a: [0b11]int";
                const ParserFixture pf{input};
                errors = {"UNEXPECTED_ARRAY_SIZE_TOKEN [Ln 1, Col 9]",
                          "No prefix parse function for RBRACKET found [Ln 1, Col 13]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 14]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect token integer type") {
                const char*         input = "var a: [0uz]int";
                const ParserFixture pf{input};
                errors = {"EMPTY_ARRAY [Ln 1, Col 9]",
                          "No prefix parse function for RBRACKET found [Ln 1, Col 12]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 13]"};
                pf.check_errors(errors);
            }

            SECTION("Incorrect token integer type") {
                const char*         input = "var a: [2uz, 0uz]int";
                const ParserFixture pf{input};
                errors = {"EMPTY_ARRAY [Ln 1, Col 14]",
                          "No prefix parse function for RBRACKET found [Ln 1, Col 17]",
                          "No prefix parse function for INT_TYPE found [Ln 1, Col 18]"};
                pf.check_errors(errors);
            }
        }
    }

    SECTION("Malformed array expressions") {
        std::vector<std::string> errors;
        SECTION("Missing size token") {
            const char*         input = "[]{1, 2, 3, }";
            const ParserFixture pf{input};
            errors = {"MISSING_ARRAY_SIZE_TOKEN [Ln 1, Col 2]",
                      "No prefix parse function for COMMA found [Ln 1, Col 5]",
                      "No prefix parse function for COMMA found [Ln 1, Col 8]",
                      "No prefix parse function for COMMA found [Ln 1, Col 11]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 13]"};
            pf.check_errors(errors);
        }

        SECTION("Incorrect token type") {
            const char*         input = R"(["wrong"]{1, 2, 3, })";
            const ParserFixture pf{input};
            errors = {"UNEXPECTED_ARRAY_SIZE_TOKEN [Ln 1, Col 2]",
                      "No prefix parse function for RBRACKET found [Ln 1, Col 9]",
                      "No prefix parse function for COMMA found [Ln 1, Col 12]",
                      "No prefix parse function for COMMA found [Ln 1, Col 15]",
                      "No prefix parse function for COMMA found [Ln 1, Col 18]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 20]"};
            pf.check_errors(errors);
        }

        SECTION("Incorrect token integer type") {
            const char*         input = "[0b11]{1, 2, 3, }";
            const ParserFixture pf{input};
            errors = {"UNEXPECTED_ARRAY_SIZE_TOKEN [Ln 1, Col 2]",
                      "No prefix parse function for RBRACKET found [Ln 1, Col 6]",
                      "No prefix parse function for COMMA found [Ln 1, Col 9]",
                      "No prefix parse function for COMMA found [Ln 1, Col 12]",
                      "No prefix parse function for COMMA found [Ln 1, Col 15]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 17]"};
            pf.check_errors(errors);
        }

        SECTION("Incorrect token value") {
            const char*         input = "[23uz]{1, 2, 3, }";
            const ParserFixture pf{input};
            errors = {"INCORRECT_EXPLICIT_ARRAY_SIZE [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Empty arrays") {
            const auto inputs = std::array{"[0uz]{}", "[_]{}"};

            errors = {"EMPTY_ARRAY [Ln 1, Col 1]"};
            for (const auto& input : inputs) {
                const ParserFixture pf{input};
                pf.check_errors(errors);
            }
        }
    }
}

TEST_CASE("Discard statements") {
    SECTION("Correct discards") {
        const char*         input = "_ = 90";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* discard_stmt = reinterpret_cast<const DiscardStatement*>(stmt);
        test_number_expression<int64_t>(discard_stmt->to_discard, 90);
    }

    SECTION("Incorrect discards") {
        const char*         input = "_ = const a := 2";
        const ParserFixture pf{input};
        const auto          errors = std::to_array<std::string>(
            {"No prefix parse function for CONST found [Ln 1, Col 5]",
                      "No prefix parse function for WALRUS found [Ln 1, Col 13]"});
        pf.check_errors(errors);
    }
}

TEST_CASE("For loops") {
    SECTION("Iterable only") {
        const char*         input = "for (1) {1}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* for_loop  = reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

        REQUIRE(for_loop->iterables.length == 1);
        REQUIRE(for_loop->captures.length == 0);
        REQUIRE(for_loop->block->statements.length == 1);
        REQUIRE_FALSE(for_loop->non_break);

        Expression* iterable;
        REQUIRE(STATUS_OK(array_list_get(&for_loop->iterables, 0, static_cast<void*>(&iterable))));
        test_number_expression<int64_t>(iterable, 1);

        REQUIRE(
            STATUS_OK(array_list_get(&for_loop->block->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_number_expression<int64_t>(expr_stmt->expression, 1);
    }

    SECTION("Iterable and capture only") {
        SECTION("Aligned captures") {
            const char*         input = "for (1) : (name) {1}";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* for_loop =
                reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

            REQUIRE(for_loop->iterables.length == 1);
            REQUIRE(for_loop->captures.length == 1);
            REQUIRE(for_loop->block->statements.length == 1);
            REQUIRE_FALSE(for_loop->non_break);

            Expression* iterable;
            REQUIRE(
                STATUS_OK(array_list_get(&for_loop->iterables, 0, static_cast<void*>(&iterable))));
            test_number_expression<int64_t>(iterable, 1);

            ForLoopCapture capture;
            REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 0, &capture)));
            REQUIRE_FALSE(capture.is_ref);
            test_identifier_expression(capture.capture, "name");

            REQUIRE(STATUS_OK(
                array_list_get(&for_loop->block->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);
        }

        SECTION("Ignored capture") {
            const char*         input = "for (1, 2) : (name, _) {1}";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* for_loop =
                reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

            REQUIRE(for_loop->iterables.length == 2);
            REQUIRE(for_loop->captures.length == 2);
            REQUIRE(for_loop->block->statements.length == 1);
            REQUIRE_FALSE(for_loop->non_break);

            ForLoopCapture capture;
            REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 0, &capture)));
            REQUIRE_FALSE(capture.is_ref);
            test_identifier_expression(capture.capture, "name");
        }
    }

    SECTION("Full for loops") {
        SECTION("With else block statement") {
            const char*         input = "for (1, 2) : (name, word) {1} else {1}";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* for_loop =
                reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

            REQUIRE(for_loop->iterables.length == 2);
            REQUIRE(for_loop->captures.length == 2);
            REQUIRE(for_loop->block->statements.length == 1);
            REQUIRE(for_loop->non_break);

            Expression* iterable;
            REQUIRE(
                STATUS_OK(array_list_get(&for_loop->iterables, 0, static_cast<void*>(&iterable))));
            test_number_expression<int64_t>(iterable, 1);
            REQUIRE(
                STATUS_OK(array_list_get(&for_loop->iterables, 1, static_cast<void*>(&iterable))));
            test_number_expression<int64_t>(iterable, 2);

            ForLoopCapture capture;
            REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 0, &capture)));
            REQUIRE_FALSE(capture.is_ref);
            test_identifier_expression(capture.capture, "name");
            REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 1, &capture)));
            REQUIRE_FALSE(capture.is_ref);
            test_identifier_expression(capture.capture, "word");

            REQUIRE(STATUS_OK(
                array_list_get(&for_loop->block->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);

            const auto* non_break = reinterpret_cast<const BlockStatement*>(for_loop->non_break);
            REQUIRE(non_break->statements.length == 1);
            REQUIRE(
                STATUS_OK(array_list_get(&non_break->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);
        }

        SECTION("With expression else") {
            const char*         input = "for (1, 2) : (name, word) {1} else 1";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* for_loop =
                reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

            const auto* non_break_expr_stmt =
                reinterpret_cast<const ExpressionStatement*>(for_loop->non_break);
            test_number_expression<int64_t>(non_break_expr_stmt->expression, 1);
        }
    }

    SECTION("Capture refs") {
        const char*         input = "for (1, 2, 3) : (name, ref hey, word) {1}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* for_loop  = reinterpret_cast<const ForLoopExpression*>(expr_stmt->expression);

        ForLoopCapture capture;
        REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 0, &capture)));
        REQUIRE_FALSE(capture.is_ref);
        test_identifier_expression(capture.capture, "name");
        REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 1, &capture)));
        REQUIRE(capture.is_ref);
        test_identifier_expression(capture.capture, "hey");
        REQUIRE(STATUS_OK(array_list_get(&for_loop->captures, 2, &capture)));
        REQUIRE_FALSE(capture.is_ref);
        test_identifier_expression(capture.capture, "word");
    }

    SECTION("Malformed for loops") {
        std::array<std::string, 1> errors;
        SECTION("Missing iterables") {
            const char*         input = "for () {}";
            const ParserFixture pf{input};
            errors = {"FOR_MISSING_ITERABLES [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Capture mismatch") {
            const char*         input = "for (1) : () {}";
            const ParserFixture pf{input};
            errors = {"FOR_ITERABLE_CAPTURE_MISMATCH [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Missing body") {
            const char*         input = "for (1) {}";
            const ParserFixture pf{input};
            errors = {"EMPTY_FOR_LOOP [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Improper non break clause") {
            const char*         input = "for (1) : (1) {1} else const a := 2;";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_LOOP_NON_BREAK [Ln 1, Col 24]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("While loops") {
    SECTION("Condition only") {
        const char*         input = "while (1) {1}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* while_loop =
            reinterpret_cast<const WhileLoopExpression*>(expr_stmt->expression);

        REQUIRE(while_loop->condition);
        REQUIRE_FALSE(while_loop->continuation);
        REQUIRE(while_loop->block->statements.length == 1);
        REQUIRE_FALSE(while_loop->non_break);

        test_number_expression<int64_t>(while_loop->condition, 1);
        REQUIRE(STATUS_OK(
            array_list_get(&while_loop->block->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_number_expression<int64_t>(expr_stmt->expression, 1);
    }

    SECTION("Condition and continuation only") {
        const char*         input = "while (1) : (1) {1}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* while_loop =
            reinterpret_cast<const WhileLoopExpression*>(expr_stmt->expression);

        REQUIRE(while_loop->condition);
        REQUIRE(while_loop->continuation);
        REQUIRE(while_loop->block->statements.length == 1);
        REQUIRE_FALSE(while_loop->non_break);

        test_number_expression<int64_t>(while_loop->condition, 1);
        test_number_expression<int64_t>(while_loop->continuation, 1);

        REQUIRE(STATUS_OK(
            array_list_get(&while_loop->block->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_number_expression<int64_t>(expr_stmt->expression, 1);
    }

    SECTION("Full while loops") {
        SECTION("With block else") {
            const char*         input = "while (1) : (1) {1} else {1}";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* while_loop =
                reinterpret_cast<const WhileLoopExpression*>(expr_stmt->expression);

            REQUIRE(while_loop->condition);
            REQUIRE(while_loop->continuation);
            REQUIRE(while_loop->block->statements.length == 1);
            REQUIRE(while_loop->non_break);

            test_number_expression<int64_t>(while_loop->condition, 1);
            test_number_expression<int64_t>(while_loop->continuation, 1);

            REQUIRE(STATUS_OK(
                array_list_get(&while_loop->block->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);

            const auto* non_break = reinterpret_cast<const BlockStatement*>(while_loop->non_break);
            REQUIRE(non_break->statements.length == 1);
            REQUIRE(
                STATUS_OK(array_list_get(&non_break->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);
        }

        SECTION("With expression else") {
            const char*         input = "while (1) : (1) {1} else 1u";
            const ParserFixture pf{input};
            pf.check_errors();

            const auto* ast = pf.ast();
            REQUIRE(ast->statements.length == 1);

            Statement* stmt;
            REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
            const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            const auto* while_loop =
                reinterpret_cast<const WhileLoopExpression*>(expr_stmt->expression);

            REQUIRE(while_loop->condition);
            REQUIRE(while_loop->continuation);
            REQUIRE(while_loop->block->statements.length == 1);
            REQUIRE(while_loop->non_break);

            test_number_expression<int64_t>(while_loop->condition, 1);
            test_number_expression<int64_t>(while_loop->continuation, 1);

            REQUIRE(STATUS_OK(
                array_list_get(&while_loop->block->statements, 0, static_cast<void*>(&stmt))));
            expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
            test_number_expression<int64_t>(expr_stmt->expression, 1);

            const auto* non_break =
                reinterpret_cast<const ExpressionStatement*>(while_loop->non_break);
            test_number_expression<uint64_t>(non_break->expression, 1);
        }
    }

    SECTION("Malformed while loops") {
        std::array<std::string, 1> errors;
        SECTION("Missing condition") {
            const char*         input = "while () {}";
            const ParserFixture pf{input};
            errors = {"WHILE_MISSING_CONDITION [Ln 1, Col 8]"};
            pf.check_errors(errors);
        }

        SECTION("Empty continuation") {
            const char*         input = "while (1) : () {}";
            const ParserFixture pf{input};
            errors = {"IMPROPER_WHILE_CONTINUATION [Ln 1, Col 9]"};
            pf.check_errors(errors);
        }

        SECTION("Missing body") {
            const char*         input = "while (1) {}";
            const ParserFixture pf{input};
            errors = {"EMPTY_WHILE_LOOP [Ln 1, Col 1]"};
            pf.check_errors(errors);
        }

        SECTION("Improper non break clause") {
            const char*         input = "while (1) : (1) {1} else const a := 2;";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_LOOP_NON_BREAK [Ln 1, Col 26]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Do while loops") {
    SECTION("Full do while loops") {
        const char*         input = "do {1} while (1)";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* do_while_loop =
            reinterpret_cast<const DoWhileLoopExpression*>(expr_stmt->expression);

        REQUIRE(do_while_loop->block->statements.length == 1);
        REQUIRE(do_while_loop->condition);

        REQUIRE(STATUS_OK(
            array_list_get(&do_while_loop->block->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_number_expression<int64_t>(expr_stmt->expression, 1);
        test_number_expression<int64_t>(do_while_loop->condition, 1);
    }

    SECTION("Malformed do while loops") {
        std::vector<std::string> errors;
        SECTION("Missing condition") {
            const char*         input = "do {1} while ()";
            const ParserFixture pf{input};
            errors = {"WHILE_MISSING_CONDITION [Ln 1, Col 15]"};
            pf.check_errors(errors);
        }

        SECTION("Missing body") {
            const char*         input = "do {} while (1)";
            const ParserFixture pf{input};
            errors = {"EMPTY_WHILE_LOOP [Ln 1, Col 1]",
                      "Expected token LBRACE, found END [Ln 1, Col 16]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Raw loops") {
    SECTION("Correct loops") {
        const char*         input = "loop {1}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* loop      = reinterpret_cast<const LoopExpression*>(expr_stmt->expression);

        REQUIRE(loop->block->statements.length == 1);
        REQUIRE(STATUS_OK(array_list_get(&loop->block->statements, 0, static_cast<void*>(&stmt))));
        expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        test_number_expression<int64_t>(expr_stmt->expression, 1);
    }

    SECTION("Malformed loop without body") {
        const char*         input = "loop {}";
        const ParserFixture pf{input};
        const auto          errors = std::to_array<std::string>({"EMPTY_LOOP [Ln 1, Col 1]"});
        pf.check_errors(errors);
    }
}

TEST_CASE("Generics") {
    SECTION("Function definition generics") {
        const char*         input = "fn<T, B>(a: int): int {}";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* function  = reinterpret_cast<const FunctionExpression*>(expr_stmt->expression);

        REQUIRE(function->generics.length == 2);

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&function->generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");
        REQUIRE(STATUS_OK(array_list_get(&function->generics, 1, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "B");
    }

    SECTION("Function type generics") {
        const char*         input = "var a: fn<T>(b: int): Result<int>;";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        test_decl_statement(stmt,
                            false,
                            "a",
                            false,
                            false,
                            TypeExpressionTag::EXPLICIT,
                            ExplicitTypeTag::EXPLICIT_FN,
                            {});

        const auto* decl_stmt   = reinterpret_cast<const DeclStatement*>(stmt);
        const auto  explicit_fn = decl_stmt->type->variant.explicit_type.variant.function_type;

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&explicit_fn.generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");

        test_type_expression(reinterpret_cast<const Expression*>(explicit_fn.return_type),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_IDENT,
                             "Result");
        REQUIRE(STATUS_OK(array_list_get(
            &explicit_fn.return_type->variant.explicit_type.variant.ident_type.generics,
            0,
            static_cast<void*>(&generic))));
        test_identifier_expression(generic, "int", TokenType::INT_TYPE);
    }

    SECTION("Function generics in type decls") {
        const char*         input = "type F = fn<T, B>(a: int): int";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* type_decl = reinterpret_cast<const TypeDeclStatement*>(stmt);
        REQUIRE_FALSE(type_decl->primitive_alias);
        test_identifier_expression(reinterpret_cast<const Expression*>(type_decl->ident), "F");

        const auto* function_type = reinterpret_cast<const TypeExpression*>(type_decl->value);
        test_type_expression(reinterpret_cast<const Expression*>(function_type),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_FN,
                             {});

        const auto function = function_type->variant.explicit_type.variant.function_type;
        REQUIRE(function.generics.length == 2);

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&function.generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");
        REQUIRE(STATUS_OK(array_list_get(&function.generics, 1, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "B");
    }

    SECTION("Struct generics") {
        const char*         input = "struct<T, E>{a: int, }";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt   = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* struct_expr = reinterpret_cast<const StructExpression*>(expr_stmt->expression);

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&struct_expr->generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");
        REQUIRE(STATUS_OK(array_list_get(&struct_expr->generics, 1, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "E");
    }

    SECTION("Call generics in type decls") {
        const char*         input = "type S = struct<T, E>{a: int, }";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* type_decl = reinterpret_cast<const TypeDeclStatement*>(stmt);
        REQUIRE_FALSE(type_decl->primitive_alias);
        test_identifier_expression(reinterpret_cast<const Expression*>(type_decl->ident), "S");

        const auto* struct_type = reinterpret_cast<const TypeExpression*>(type_decl->value);
        test_type_expression(reinterpret_cast<const Expression*>(struct_type),
                             false,
                             false,
                             TypeExpressionTag::EXPLICIT,
                             ExplicitTypeTag::EXPLICIT_STRUCT,
                             {});

        const auto* struct_expr = struct_type->variant.explicit_type.variant.struct_type;
        REQUIRE(struct_expr->generics.length == 2);

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&struct_expr->generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "T");
        REQUIRE(STATUS_OK(array_list_get(&struct_expr->generics, 1, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "E");
    }

    SECTION("Call generics") {
        const char*         input = "func(1, 2) with <int>";
        const ParserFixture pf{input};
        pf.check_errors();

        const auto* ast = pf.ast();
        REQUIRE(ast->statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, static_cast<void*>(&stmt))));
        const auto* expr_stmt = reinterpret_cast<const ExpressionStatement*>(stmt);
        const auto* call_expr = reinterpret_cast<const CallExpression*>(expr_stmt->expression);

        Expression* generic;
        REQUIRE(STATUS_OK(array_list_get(&call_expr->generics, 0, static_cast<void*>(&generic))));
        test_identifier_expression(generic, "int", TokenType::INT_TYPE);
    }

    SECTION("Malformed generics") {
        std::vector<std::string> errors;
        SECTION("Incorrect generic tokens") {
            const char*         input = "struct<1>{a: int,}";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_IDENTIFIER [Ln 1, Col 8]",
                      "No prefix parse function for GT found [Ln 1, Col 9]",
                      "No prefix parse function for COLON found [Ln 1, Col 12]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 14]",
                      "No prefix parse function for COMMA found [Ln 1, Col 17]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 18]"};
            pf.check_errors(errors);
        }

        SECTION("Empty generic list in structs") {
            const char*         input = "struct<>{a: int,}";
            const ParserFixture pf{input};
            errors = {"EMPTY_GENERIC_LIST [Ln 1, Col 8]",
                      "No prefix parse function for COLON found [Ln 1, Col 11]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 13]",
                      "No prefix parse function for COMMA found [Ln 1, Col 16]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 17]"};
            pf.check_errors(errors);
        }

        SECTION("Incorrect generic expression") {
            const char*         input = R"(struct<"2" + 2>{a: int,})";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_IDENTIFIER [Ln 1, Col 8]",
                      "No prefix parse function for PLUS found [Ln 1, Col 12]",
                      "No prefix parse function for LBRACE found [Ln 1, Col 16]",
                      "No prefix parse function for COLON found [Ln 1, Col 18]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 20]",
                      "No prefix parse function for COMMA found [Ln 1, Col 23]",
                      "No prefix parse function for RBRACE found [Ln 1, Col 24]"};
            pf.check_errors(errors);
        }

        SECTION("Empty generic list in functions") {
            const char*         input = "var a: fn<>(a: int): int";
            const ParserFixture pf{input};
            errors = {"EMPTY_GENERIC_LIST [Ln 1, Col 11]",
                      "Expected token RPAREN, found COLON [Ln 1, Col 14]",
                      "No prefix parse function for COLON found [Ln 1, Col 14]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 16]",
                      "No prefix parse function for RPAREN found [Ln 1, Col 19]",
                      "No prefix parse function for COLON found [Ln 1, Col 20]",
                      "No prefix parse function for INT_TYPE found [Ln 1, Col 22]"};
            pf.check_errors(errors);
        }

        SECTION("Missing with clause") {
            const char*         input = "func(1, 2) <int>";
            const ParserFixture pf{input};
            errors = {"No prefix parse function for INT_TYPE found [Ln 1, Col 13]",
                      "No prefix parse function for GT found [Ln 1, Col 16]"};
            pf.check_errors(errors);
        }

        SECTION("Empty generic list in calls") {
            const char*         input = "func(1, 2) with <>";
            const ParserFixture pf{input};
            errors = {"EMPTY_GENERIC_LIST [Ln 1, Col 18]"};
            pf.check_errors(errors);
        }

        SECTION("With clause with wrong tokens") {
            const char*         input = R"(func(1, 2) with <int, "2" + 2>)";
            const ParserFixture pf{input};
            errors = {"ILLEGAL_IDENTIFIER [Ln 1, Col 23]",
                      "No prefix parse function for PLUS found [Ln 1, Col 27]",
                      "INFIX_MISSING_RHS [Ln 1, Col 30]"};
            pf.check_errors(errors);
        }
    }
}

TEST_CASE("Null passed to destructors") {
    const auto destructors = std::array{
        array_literal_expression_destroy,
        assignment_expression_destroy,
        bool_literal_expression_destroy,
        call_expression_destroy,
        index_expression_destroy,
        enum_expression_destroy,
        float_literal_expression_destroy,
        function_expression_destroy,
        identifier_expression_destroy,
        if_expression_destroy,
        infix_expression_destroy,
        integer_expression_destroy,
        for_loop_expression_destroy,
        while_loop_expression_destroy,
        do_while_loop_expression_destroy,
        loop_expression_destroy,
        match_expression_destroy,
        namespace_expression_destroy,
        prefix_expression_destroy,
        single_expression_destroy,
        string_literal_expression_destroy,
        struct_expression_destroy,
        type_expression_destroy,
        block_statement_destroy,
        decl_statement_destroy,
        type_decl_statement_destroy,
        discard_statement_destroy,
        expression_statement_destroy,
        impl_statement_destroy,
        import_statement_destroy,
        jump_statement_destroy,
    };

    for (const auto d : destructors) {
        d(nullptr, &std_allocator);
    }
}
