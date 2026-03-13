var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/index.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"1472",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IndexExpression::parse(Parser& parser, Box<Expression> array)"},
{"lineNum":"   10","line":"    -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1469",},
{"lineNum":"   11","line":"    const auto start_token = array->get_token();","class":"lineCov","hits":"1","order":"1468",},
{"lineNum":"   12","line":"    if (parser.peek_token_is(TokenType::RBRACKET)) {","class":"lineCov","hits":"1","order":"1470",},
{"lineNum":"   13","line":"        return make_parser_unexpected(ParserError::INDEX_MISSING_EXPRESSION, start_token);","class":"lineCov","hits":"1","order":"1467",},
{"lineNum":"   14","line":"    }"},
{"lineNum":"   15","line":"    parser.advance();","class":"lineCov","hits":"1","order":"1466",},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"    auto idx_expr = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1465",},
{"lineNum":"   18","line":"    TRY(parser.expect_peek(TokenType::RBRACKET));","class":"lineCov","hits":"1","order":"1471",},
{"lineNum":"   19","line":"    return make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));","class":"lineCov","hits":"1","order":"1463",},
{"lineNum":"   20","line":"}","class":"lineCov","hits":"1","order":"1464",},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 10, "covered" : 10,};
var merged_data = [];
