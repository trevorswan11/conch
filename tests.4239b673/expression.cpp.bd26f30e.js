var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/expression.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto ExpressionStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"148","possible_hits":"1",},
{"lineNum":"   10","line":"    const auto start_token = parser.current_token();","class":"lineCov","hits":"1","order":"149","possible_hits":"1",},
{"lineNum":"   11","line":"    auto       expr        = TRY(parser.parse_expression());","class":"linePartCov","hits":"1","order":"151","possible_hits":"3",},
{"lineNum":"   12","line":""},
{"lineNum":"   13","line":"    return make_box<ExpressionStatement>(start_token, std::move(expr));","class":"lineCov","hits":"1","order":"212","possible_hits":"1",},
{"lineNum":"   14","line":"}","class":"linePartCov","hits":"2","order":"216","possible_hits":"4",},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 6, "covered" : 5,};
var merged_data = [];
