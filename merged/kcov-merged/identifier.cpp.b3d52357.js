var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/identifier.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"111",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IdentifierExpression::parse(Parser& parser) // cppcheck-suppress constParameterReference"},
{"lineNum":"   10","line":"    -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"109",},
{"lineNum":"   11","line":"    const auto start_token = parser.current_token();","class":"lineCov","hits":"1","order":"108",},
{"lineNum":"   12","line":"    if (!start_token.is_valid_ident()) {","class":"lineCov","hits":"1","order":"110",},
{"lineNum":"   13","line":"        return make_parser_unexpected(ParserError::ILLEGAL_IDENTIFIER, start_token);","class":"lineCov","hits":"1","order":"107",},
{"lineNum":"   14","line":"    }"},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"    return make_box<IdentifierExpression>(start_token);","class":"lineCov","hits":"1","order":"106",},
{"lineNum":"   17","line":"}","class":"lineCov","hits":"1","order":"105",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 7, "covered" : 7,};
var merged_data = [];
