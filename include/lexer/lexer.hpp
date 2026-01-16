#pragma once

#include <string>

auto Foo() -> int;

class Lexer {
  public:
  private:
    std::string             input_;
    [[maybe_unused]] size_t pos_;
    [[maybe_unused]] size_t peek_pos_;
    [[maybe_unused]] char   current_byte_;

    [[maybe_unused]] size_t line_no_;
    [[maybe_unused]] size_t col_no_;
};
