var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/jump.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"visitor/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineNoCov","hits":"0","possible_hits":"3",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto JumpStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   10","line":"    const auto start_token = parser.current_token();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"    Optional<Box<Expression>> value;","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   13","line":"    if (start_token.type != TokenType::CONTINUE && !parser.peek_token_is(TokenType::END) &&","class":"lineNoCov","hits":"0","possible_hits":"2",},
{"lineNum":"   14","line":"        !parser.peek_token_is(TokenType::SEMICOLON)) {","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   15","line":"        parser.advance();","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   16","line":"        value = TRY(parser.parse_expression());","class":"lineNoCov","hits":"0","possible_hits":"4",},
{"lineNum":"   17","line":"    }","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"    return make_box<JumpStatement>(start_token, std::move(value));","class":"lineNoCov","hits":"0","possible_hits":"1",},
{"lineNum":"   20","line":"}","class":"lineNoCov","hits":"0","possible_hits":"4",},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "tests", "date" : "2026-02-11 22:34:36", "instrumented" : 11, "covered" : 0,};
var merged_data = [];
