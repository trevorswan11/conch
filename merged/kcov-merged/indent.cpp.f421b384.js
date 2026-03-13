var data = {lines:[
{"lineNum":"    1","line":"#include \"indent.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"namespace conch {"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"auto Indent::current_branch() const -> std::string {","class":"lineCov","hits":"1","order":"13",},
{"lineNum":"    6","line":"    if (levels_.empty()) { return \"\"; }","class":"lineCov","hits":"1","order":"11",},
{"lineNum":"    7","line":"    std::string res = prefix_only();","class":"lineCov","hits":"1","order":"10",},
{"lineNum":"    8","line":"    res += (levels_.back() ? symbols::L_BRANCH : symbols::T_BRANCH);","class":"lineCov","hits":"1","order":"9",},
{"lineNum":"    9","line":"    return res;","class":"lineCov","hits":"1","order":"8",},
{"lineNum":"   10","line":"}","class":"lineCov","hits":"1","order":"7",},
{"lineNum":"   11","line":""},
{"lineNum":"   12","line":"auto Indent::prefix_only() const -> std::string {","class":"lineCov","hits":"1","order":"6",},
{"lineNum":"   13","line":"    std::string res;","class":"lineCov","hits":"1","order":"5",},
{"lineNum":"   14","line":"    for (size_t i = 0; i + 1 < levels_.size(); ++i) {","class":"lineCov","hits":"1","order":"4",},
{"lineNum":"   15","line":"        res += (levels_[i] ? symbols::EMPTY : symbols::VERT_BAR);","class":"lineCov","hits":"1","order":"3",},
{"lineNum":"   16","line":"    }","class":"lineCov","hits":"1","order":"12",},
{"lineNum":"   17","line":"    return res;","class":"lineCov","hits":"1","order":"1",},
{"lineNum":"   18","line":"}","class":"lineCov","hits":"1","order":"2",},
{"lineNum":"   19","line":""},
{"lineNum":"   20","line":"} // namespace conch"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-03-13 16:31:19", "instrumented" : 13, "covered" : 13,};
var merged_data = [];
