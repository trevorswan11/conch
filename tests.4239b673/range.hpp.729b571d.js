var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/expressions/infix.hpp\""},
{"lineNum":"    4","line":"#include \"ast/node.hpp\""},
{"lineNum":"    5","line":""},
{"lineNum":"    6","line":"namespace conch::ast {"},
{"lineNum":"    7","line":""},
{"lineNum":"    8","line":"class RangeExpression : public InfixExpression<RangeExpression> {","class":"lineNoCov","hits":"0","possible_hits":"6",},
{"lineNum":"    9","line":"  public:"},
{"lineNum":"   10","line":"    static constexpr auto KIND = NodeKind::RANGE_EXPRESSION;"},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"  public:"},
{"lineNum":"   13","line":"    using InfixExpression::InfixExpression;","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   14","line":"    using InfixExpression::parse;"},
{"lineNum":"   15","line":"};"},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 2, "covered" : 0,};
var merged_data = [];
