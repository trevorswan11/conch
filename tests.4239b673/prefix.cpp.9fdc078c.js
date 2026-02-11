var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/prefix.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto PrefixExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto PrefixExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   10","line":"    const auto prefix_token = parser.current_token();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   11","line":"    if (parser.peek_token_is(TokenType::END)) {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   12","line":"        return make_parser_unexpected(ParserError::PREFIX_MISSING_OPERAND, prefix_token);","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   13","line":"    }"},
{"lineNum":"   14","line":"    parser.advance();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"    auto operand = TRY(parser.parse_expression(Precedence::PREFIX));","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   17","line":"    return make_box<PrefixExpression>(prefix_token, std::move(operand));","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   18","line":"}","class":"lineNoCov","hits":"0","possible_hits":"4",},
{"lineNum":"   19","line":""},
{"lineNum":"   20","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 9, "covered" : 0,};
var merged_data = [];
