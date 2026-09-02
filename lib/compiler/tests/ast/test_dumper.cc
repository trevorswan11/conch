#include <sstream>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <stdx/types.hh>

#include "compiler/ast/dumper.hh"
#include "compiler/syntax/error.hh"
#include "compiler/syntax/parser.hh"
#include "helpers/common.hh"

namespace ghoti::tests {

constexpr std::string_view golden_input{
#include "ast/golden.gh.inc"
};

constexpr std::string_view expected{
#include "ast/dump.inc"
};

namespace {

[[nodiscard]] auto dump_source(std::string_view src) -> std::string {
    syntax::parser p{src};
    ast::AST       ast;
    auto           errors{p.consume(ast)};
    helpers::check_errors<syntax::diagnostic>(errors);

    std::ostringstream oss;
    ast::dumper        dumper{ast, oss};
    dumper.dump();
    return oss.str();
}

[[nodiscard]] auto count_substr(std::string_view haystack, std::string_view needle) -> usize {
    usize n{0};
    for (auto pos{haystack.find(needle)}; pos != std::string_view::npos;
         pos = haystack.find(needle, pos + needle.size())) {
        ++n;
    }
    return n;
}

} // namespace

TEST_CASE("Comprehensive dump") {
    syntax::parser p{golden_input};
    ast::AST       ast;
    auto           errors{p.consume(ast)};
    helpers::check_errors<syntax::diagnostic>(errors);

    std::ostringstream oss;
    ast::dumper        dumper{ast, oss};
    dumper.dump();
    CHECK(expected == oss.view());
}

TEST_CASE("Dumps a range pattern as a match arm") {
    const auto out{dump_source("match (n) { 1..8 => |v| v, _ => 0 };")};
    CHECK(out.find("MatchExpression") != std::string::npos);
    CHECK(out.find("RangeExpression") != std::string::npos);
    // One pattern under the range arm, one under the catch-all arm.
    CHECK(count_substr(out, "Pattern: ") == 2);
}

} // namespace ghoti::tests
