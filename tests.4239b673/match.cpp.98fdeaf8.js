var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/match.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto MatchExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   10","line":"    TODO(parser);","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   11","line":"}"},
{"lineNum":"   12","line":""},
{"lineNum":"   13","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 3, "covered" : 0,};
var merged_data = [];
