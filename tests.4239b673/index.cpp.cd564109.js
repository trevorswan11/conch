var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/index.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IndexExpression::parse(Parser& parser, Box<Expression> array)"},
{"lineNum":"   10","line":"    -> Expected<Box<Expression>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   11","line":"    const auto start_token = parser.current_token();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   12","line":"    if (parser.current_token_is(TokenType::RBRACE)) {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   13","line":"        return make_parser_unexpected(ParserError::INDEX_MISSING_EXPRESSION, start_token);","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   14","line":"    }"},
{"lineNum":"   15","line":"    parser.advance();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"    auto idx_expr = TRY(parser.parse_expression());","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   18","line":"    TRY(parser.expect_peek(TokenType::RBRACKET));","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"   19","line":"    return make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   20","line":"}","class":"lineNoCov","hits":"0","possible_hits":"6",},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 10, "covered" : 0,};
var merged_data = [];
