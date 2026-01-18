#include <cctype>

#include "lexer/keywords.hpp"
#include "lexer/lexer.hpp"

auto Lexer::reset(std::string_view input) noexcept -> void { *this = Lexer{input}; }

auto Lexer::skipWhitespace() noexcept -> void {
    while (std::isspace(current_byte_)) {
        readChar();
    }
}

auto Lexer::luIdent(std::string_view ident) noexcept -> TokenType {
    const auto it = ALL_KEYWORDS.find(ident);
    return it == ALL_KEYWORDS.end() ? TokenType::IDENT : it->second;
}

auto Lexer::readChar() noexcept -> void {
    if (peek_pos_ >= input_.size()) {
        current_byte_ = '\0';
    } else {
        current_byte_ = input_[peek_pos_];
    }

    if (current_byte_ == '\n') {
        line_no_ += 1;
        col_no_ = 0;
    } else {
        col_no_ += 1;
    }

    pos_ = peek_pos_;
    peek_pos_ += 1;
}

auto Lexer::readOperator() -> Token {
    const auto start_line = line_no_;
    const auto start_col  = col_no_;

    if (l->current_byte == '\0') {
        return token_init(END, &l->input[l->position], 0, start_line, start_col);
    }

    size_t    max_len      = 0;
    TokenType matched_type = ILLEGAL;

    // Try extending from length 1 up to the max operator size
    for (size_t len = 1; len <= MAX_OPERATOR_LEN && l->position + len <= l->input_length; len++) {
        Slice query = slice_from_str_s(&l->input[l->position], len);
        if (STATUS_OK(hash_map_get_value(&l->operators, &query, &matched_type))) { max_len = len; }
    }

    // We cannot greedily consume the lexer here since the next token instruction handles that
    if (max_len == 0) {
        return token_init(ILLEGAL, &l->input[l->position], 1, start_line, start_col);
    }
    return token_init(matched_type, &l->input[l->position], max_len, start_line, start_col);
}
