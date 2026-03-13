var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/expression.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"1159",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto ExpressionStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1158",},
{"lineNum":"   10","line":"    const auto start_token = parser.current_token();","class":"lineCov","hits":"1","order":"1157",},
{"lineNum":"   11","line":"    auto       expr        = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1156",},
{"lineNum":"   12","line":""},
{"lineNum":"   13","line":"    if (!parser.current_token_is(TokenType::SEMICOLON)) {","class":"lineCov","hits":"1","order":"1155",},
{"lineNum":"   14","line":"        TRY(parser.expect_peek(TokenType::SEMICOLON));","class":"lineCov","hits":"1","order":"1154",},
{"lineNum":"   15","line":"    }","class":"lineCov","hits":"1","order":"1152",},
{"lineNum":"   16","line":"    return make_box<ExpressionStatement>(start_token, std::move(expr));","class":"lineCov","hits":"1","order":"1151",},
{"lineNum":"   17","line":"}","class":"lineCov","hits":"1","order":"1153",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 9, "covered" : 9,};
var merged_data = [];
