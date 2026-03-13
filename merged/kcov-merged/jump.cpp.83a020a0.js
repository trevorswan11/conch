var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/jump.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace conch::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"833",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto JumpStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {","class":"lineCov","hits":"1","order":"831",},
{"lineNum":"   10","line":"    const auto start_token = parser.current_token();","class":"lineCov","hits":"1","order":"830",},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"    Optional<Box<Expression>> value;","class":"lineCov","hits":"1","order":"832",},
{"lineNum":"   13","line":"    if (start_token.type == TokenType::RETURN && !parser.peek_token_is(TokenType::END) &&","class":"lineCov","hits":"1","order":"828",},
{"lineNum":"   14","line":"        !parser.peek_token_is(TokenType::SEMICOLON)) {","class":"lineCov","hits":"1","order":"827",},
{"lineNum":"   15","line":"        parser.advance();","class":"lineCov","hits":"1","order":"826",},
{"lineNum":"   16","line":"        value = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"825",},
{"lineNum":"   17","line":"    }","class":"lineCov","hits":"1","order":"824",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"    TRY(parser.expect_peek(TokenType::SEMICOLON));","class":"lineCov","hits":"1","order":"823",},
{"lineNum":"   20","line":"    return make_box<JumpStatement>(start_token, std::move(value));","class":"lineCov","hits":"1","order":"822",},
{"lineNum":"   21","line":"}","class":"lineCov","hits":"1","order":"829",},
{"lineNum":"   22","line":""},
{"lineNum":"   23","line":"} // namespace conch::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 12, "covered" : 12,};
var merged_data = [];
