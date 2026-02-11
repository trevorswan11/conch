var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/identifier.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IdentifierExpression::parse(Parser& parser) // cppcheck-suppress constParameterReference"},
{"lineNum":"   10","line":"    -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"544","possible_hits":"1",},
{"lineNum":"   11","line":"    const auto start_token = parser.current_token();","class":"lineCov","hits":"1","order":"545","possible_hits":"1",},
{"lineNum":"   12","line":"    if (start_token.type != TokenType::IDENT && !start_token.is_primitive()) {","class":"lineCov","hits":"1","order":"546","possible_hits":"1",},
{"lineNum":"   13","line":"        return make_parser_unexpected(ParserError::ILLEGAL_IDENTIFIER, start_token);","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   14","line":"    }"},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"    return make_box<IdentifierExpression>(start_token, start_token.slice);","class":"lineCov","hits":"1","order":"547","possible_hits":"1",},
{"lineNum":"   17","line":"}","class":"lineCov","hits":"1","order":"549","possible_hits":"1",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 7, "covered" : 5,};
var merged_data = [];
