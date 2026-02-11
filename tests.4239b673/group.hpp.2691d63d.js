var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"util/common.hpp\""},
{"lineNum":"    4","line":"#include \"util/expected.hpp\""},
{"lineNum":"    5","line":""},
{"lineNum":"    6","line":"#include \"ast/node.hpp\""},
{"lineNum":"    7","line":""},
{"lineNum":"    8","line":"#include \"parser/parser.hpp\""},
{"lineNum":"    9","line":""},
{"lineNum":"   10","line":"namespace conch::ast {"},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"class GroupedExpression {"},
{"lineNum":"   13","line":"  public:"},
{"lineNum":"   14","line":"    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   15","line":"        parser.advance();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   16","line":"        auto inner = TRY(parser.parse_expression());","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   17","line":"        TRY(parser.expect_peek(TokenType::RPAREN));","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   18","line":"        return inner;","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   19","line":"    }","class":"lineNoCov","hits":"0","possible_hits":"6",},
{"lineNum":"   20","line":"};"},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 6, "covered" : 0,};
var merged_data = [];
