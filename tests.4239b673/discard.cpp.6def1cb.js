var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/discard.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto DiscardStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto DiscardStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   10","line":"    const auto start_token = parser.current_token();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"    TRY(parser.expect_peek(TokenType::ASSIGN));","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   13","line":"    parser.advance();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   14","line":"    auto expr = TRY(parser.parse_expression());","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"    return make_box<DiscardStatement>(start_token, std::move(expr));","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   17","line":"}","class":"lineNoCov","hits":"0","possible_hits":"5",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 8, "covered" : 0,};
var merged_data = [];
