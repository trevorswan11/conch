var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/node.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"#include \"parser/parser.hpp\""},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"namespace conch::ast {"},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"class GroupedExpression {"},
{"lineNum":"   10","line":"  public:"},
{"lineNum":"   11","line":"    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1728",},
{"lineNum":"   12","line":"        parser.advance();","class":"lineCov","hits":"1","order":"1726",},
{"lineNum":"   13","line":"        auto inner = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1725",},
{"lineNum":"   14","line":"        TRY(parser.expect_peek(TokenType::RPAREN));","class":"lineCov","hits":"1","order":"1724",},
{"lineNum":"   15","line":"        return inner;","class":"lineCov","hits":"1","order":"1723",},
{"lineNum":"   16","line":"    }","class":"lineCov","hits":"1","order":"1727",},
{"lineNum":"   17","line":"};"},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 6, "covered" : 6,};
var merged_data = [];
