#include <cassert>
#include <cctype>
#include <string_view>
#include <utility>

#include "lexer/keywords.hpp"
#include "lexer/lexer.hpp"
#include "lexer/operators.hpp"
#include "lexer/token.hpp"

namespace conch {

auto Lexer::reset(std::string_view input) noexcept -> void { *this = Lexer{input}; }

auto Lexer::advance() noexcept -> Token {
    skip_whitespace();

    const auto start_line = line_no_;
    const auto start_col  = col_no_;

    Token      token{{}, {}, start_line, start_col};
    const auto maybe_operator = read_operator();

    if (maybe_operator) {
        if (maybe_operator->type == TokenType::END) { return *maybe_operator; }
        for (size_t i = 0; i < maybe_operator->slice.size(); ++i) {
            read_character();
        }

        if (maybe_operator->type == TokenType::COMMENT) { return read_comment(); }
        if (maybe_operator->type == TokenType::MULTILINE_STRING) { return read_multiline_string(); }

        return *maybe_operator;
    }

    const auto maybe_misc_token_type = token_type::misc_from_char(current_byte_);
    if (maybe_misc_token_type) {
        token.slice = input_.substr(pos_, 1);
        token.type  = *maybe_misc_token_type;
    } else if (std::isalpha(current_byte_)) {
        token.slice = read_ident();
        token.type  = lu_ident(token.slice);
        return token;
    } else if (std::isdigit(current_byte_)) {
        return read_number();
    } else if (current_byte_ == '"') {
        return read_string();
    } else if (current_byte_ == '\'') {
        return read_byte_literal();
    } else {
        token = {TokenType::ILLEGAL, input_.substr(pos_, 1), start_line, start_col};
    }

    read_character();
    return token;
}

auto Lexer::consume() -> std::vector<Token> {
    reset(input_);

    std::vector<Token> tokens;
    do {
        tokens.emplace_back(advance());
    } while (tokens.back().type != TokenType::END);

    return tokens;
}

auto Lexer::skip_whitespace() noexcept -> void {
    while (std::isspace(current_byte_)) {
        read_character();
    }
}

auto Lexer::lu_ident(std::string_view ident) noexcept -> TokenType {
    return get_keyword(ident)
        .transform([](const auto& keyword) noexcept -> TokenType { return keyword.second; })
        .value_or(TokenType::IDENT);
}

auto Lexer::read_character(uint8_t n) noexcept -> void {
    for (uint8_t i = 0; i < n; ++i) {
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
}

auto Lexer::read_operator() const noexcept -> Optional<Token> {
    const auto start_line = line_no_;
    const auto start_col  = col_no_;

    if (current_byte_ == '\0') { return Token{TokenType::END, {}, start_line, start_col}; }

    size_t max_len      = 0;
    auto   matched_type = TokenType::ILLEGAL;

    // Try extending from length 1 up to the max operator size
    for (size_t len = 1; len <= MAX_OPERATOR_LEN && pos_ + len <= input_.size(); len++) {
        const auto op = get_operator(input_.substr(pos_, len));
        if (op) {
            matched_type = op->second;
            max_len      = len;
        }
    }

    // We cannot greedily consume the lexer here since the next token instruction handles that
    if (max_len == 0) { return nullopt; }
    return Token{matched_type, input_.substr(pos_, max_len), start_line, start_col};
}

auto Lexer::read_ident() noexcept -> std::string_view {
    const auto start = pos_;

    auto passed_first = false;
    while (std::isalpha(current_byte_) || current_byte_ == '_' ||
           (passed_first && std::isdigit(current_byte_))) {
        read_character();
        passed_first = true;
    }

    return input_.substr(start, pos_ - start);
}

enum class NumberSuffix {
    NONE     = 0,
    UNSIGNED = 1,
    SIZE     = 2,
};

auto Lexer::read_number() noexcept -> Token {
    const auto start           = pos_;
    const auto start_line      = line_no_;
    const auto start_col       = col_no_;
    auto       passed_decimal  = false;
    auto       passed_exponent = false;
    auto       base{Base::DECIMAL};

    // Detect numeric prefix
    if (current_byte_ == '0' && peek_pos_ < input_.size()) {
        const auto next = input_[peek_pos_];
        if (next == 'x' || next == 'X') {
            base = Base::HEXADECIMAL;
            read_character(2);
        } else if (next == 'b' || next == 'B') {
            base = Base::BINARY;
            read_character(2);
        } else if (next == 'o' || next == 'O') {
            base = Base::OCTAL;
            read_character(2);
        }
    }

    // Consume digits and handle dot/range rules
    assert(base != Base::UNKNOWN);
    while (true) {
        const auto c = current_byte_;

        // Exponent handling defaults to floats for simplicity
        if (base == Base::DECIMAL && !passed_exponent && (c == 'e' || c == 'E')) {
            auto p = peek_pos_;
            if (p >= input_.size()) { break; }

            auto next = input_[p];
            if (next == '+' || next == '-') {
                p += 1;
                if (p >= input_.size()) { break; }
                next = input_[p];
            }

            if (!std::isdigit(next)) { break; }

            passed_exponent = true;
            read_character();

            if (current_byte_ == '+' || current_byte_ == '-') { read_character(); }
            while (std::isdigit(current_byte_)) {
                read_character();
            }

            continue;
        }

        // Fractional part
        if (base == Base::DECIMAL && c == '.') {
            if (peek_pos_ < input_.size() && input_[peek_pos_] == '.') { break; }
            if (passed_decimal) { break; }

            passed_decimal = true;
            read_character();
            continue;
        }

        // Normal digit
        if (digit_in_base(c, base)) {
            read_character();
            continue;
        }

        break;
    }

    // Quick non-base-10 length validation
    if (base != Base::DECIMAL && pos_ - start <= 2) {
        return {TokenType::ILLEGAL, input_.substr(start, pos_ - start), start_line, start_col};
    }

    auto suffix{NumberSuffix::NONE};
    if (!(passed_decimal || passed_exponent) && pos_ < input_.size()) {
        auto c = current_byte_;
        if (c == 'u' || c == 'U') {
            suffix = NumberSuffix::UNSIGNED;
            read_character();

            c = current_byte_;
            if (c == 'z' || c == 'Z') {
                suffix = NumberSuffix::SIZE;
                read_character();
            }
        }
    }

    // Total validation
    const auto length = pos_ - start;
    auto       type{TokenType::ILLEGAL};
    if (length == 0) { return {type, input_.substr(start, 1), start_line, start_col}; }

    if (input_[pos_ - 1] == '.') {
        return {type, input_.substr(start, length), start_line, start_col};
    }

    if (passed_decimal && (base != Base::DECIMAL)) {
        return {type, input_.substr(start, length), start_line, start_col};
    }

    // Determine the input type
    if (passed_decimal || passed_exponent) {
        type = TokenType::FLOAT;
    } else {
        switch (suffix) {
        case NumberSuffix::NONE: type = TokenType::INT_2; break;
        case NumberSuffix::UNSIGNED: type = TokenType::UINT_2; break;
        case NumberSuffix::SIZE: type = TokenType::UZINT_2; break;
        }
        type = static_cast<TokenType>(std::to_underlying(type) + base_idx(base));
    }

    return {type, input_.substr(start, length), start_line, start_col};
}

auto Lexer::read_escape() noexcept -> byte {
    read_character();

    switch (current_byte_) {
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    case '0': return '\0';
    default: return current_byte_;
    }
}

auto Lexer::read_string() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    read_character();

    while (current_byte_ != '"' && current_byte_ != '\0') {
        if (current_byte_ == '\\') { read_escape(); }
        read_character();
    }

    if (current_byte_ == '\0') {
        return {
            TokenType::ILLEGAL,
            input_.substr(start, pos_ - start),
            start_line,
            start_col,
        };
    }
    read_character();

    return {TokenType::STRING, input_.substr(start, pos_ - start), start_line, start_col};
}

// Reads a multiline string from the token, assuming the '\\' operator has been consumed
auto Lexer::read_multiline_string() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    auto       end_pos    = start;

    while (true) {
        // Consume characters until newline or EOF
        while (current_byte_ != '\n' && current_byte_ != '\r' && current_byte_ != '\0') {
            read_character();
        }

        // Peek positions
        size_t peek_pos = peek_pos_;
        if (current_byte_ == '\r' && peek_pos < input_.size() && input_[peek_pos] == '\n') {
            peek_pos += 1;
        }

        bool has_continuation = false;
        if ((current_byte_ == '\n' || current_byte_ == '\r') && peek_pos + 1 < input_.size() &&
            input_[peek_pos] == '\\' && input_[peek_pos + 1] == '\\') {
            has_continuation = true;
        }

        // Don't include the newline if there is no continuation to prevent trailing whitespace
        if (!has_continuation) {
            end_pos = pos_;
            break;
        }

        // Include the CRLF/LF newline in the token
        read_character();
        if (current_byte_ == '\r' && peek_pos_ < input_.size() && input_[peek_pos_] == '\n') {
            read_character();
        }

        // consume the next "\\" line continuation
        read_character(2);
    }

    return {
        TokenType::MULTILINE_STRING,
        input_.substr(start, end_pos - start),
        start_line,
        start_col,
    };
}

// Reads a byte literal returning an illegal token for malformed literals.
//
// Assumes that the surrounding single quotes have not been consumed.
auto Lexer::read_byte_literal() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    read_character();

    // Consume one logical character
    if (current_byte_ == '\\') {
        read_escape();
        read_character();
    } else if (current_byte_ != '\'' && current_byte_ != '\n' && current_byte_ != '\r') {
        read_character();
    } else {
        return {TokenType::ILLEGAL, input_.substr(start, pos_ - start), start_line, start_col};
    }

    // The next character MUST be closing ', otherwise illegally consume like a comment
    if (current_byte_ != '\'') {
        auto illegal_end = pos_;
        while (current_byte_ != '\'' && current_byte_ != '\n' && current_byte_ != '\r' &&
               current_byte_ != '\0') {
            read_character();
            illegal_end = pos_;
        }

        if (current_byte_ == '\'') {
            read_character();
            illegal_end = pos_;
        }

        return {
            TokenType::ILLEGAL,
            input_.substr(start, illegal_end - start),
            start_line,
            start_col,
        };
    }
    read_character();

    return {TokenType::CHARACTER, input_.substr(start, pos_ - start), start_line, start_col};
}

// Reads a comment from the token, assuming the '//' operator has been consumed
auto Lexer::read_comment() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    while (current_byte_ != '\n' && current_byte_ != '\0') {
        read_character();
    }

    return {TokenType::COMMENT, input_.substr(start, pos_ - start), start_line, start_col};
}

} // namespace conch
