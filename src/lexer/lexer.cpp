#include <cassert>
#include <cctype>
#include <string_view>
#include <utility>

#include "core.hpp"

#include "lexer/keywords.hpp"
#include "lexer/lexer.hpp"
#include "lexer/operators.hpp"
#include "lexer/token.hpp"

auto Lexer::reset(std::string_view input) noexcept -> void { *this = Lexer{input}; }

auto Lexer::advance() noexcept -> Token {
    skipWhitespace();

    const auto start_line = line_no_;
    const auto start_col  = col_no_;

    Token token{
        .type   = {},
        .slice  = {},
        .line   = start_line,
        .column = start_col,
    };
    const auto maybe_operator = readOperator();

    if (maybe_operator) {
        if (maybe_operator->type == TokenType::END) { return *maybe_operator; }
        for (size_t i = 0; i < maybe_operator->slice.size(); ++i) {
            readInputCharacter();
        }

        if (maybe_operator->type == TokenType::COMMENT) { return readComment(); }
        if (maybe_operator->type == TokenType::MULTILINE_STRING) { return readMultilineString(); }

        return *maybe_operator;
    }

    const auto maybe_misc_token_type = token_type::miscFromChar(current_byte_);
    if (maybe_misc_token_type) {
        token.slice = input_.substr(pos_, 1);
        token.type  = *maybe_misc_token_type;
    } else if (std::isalpha(current_byte_)) {
        token.slice = readIdent();
        token.type  = luIdent(token.slice);
        return token;
    } else if (std::isdigit(current_byte_)) {
        return readNumber();
    } else if (current_byte_ == '"') {
        return readString();
    } else if (current_byte_ == '\'') {
        return readByteLiteral();
    } else {
        token = {
            .type   = TokenType::ILLEGAL,
            .slice  = input_.substr(pos_, 1),
            .line   = start_line,
            .column = start_col,
        };
    }

    readInputCharacter();
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

auto Lexer::skipWhitespace() noexcept -> void {
    while (std::isspace(current_byte_)) {
        readInputCharacter();
    }
}

auto Lexer::luIdent(std::string_view ident) noexcept -> TokenType {
    return get_keyword(ident)
        .transform([](const auto& keyword) noexcept -> TokenType { return keyword.second; })
        .value_or(TokenType::IDENT);
}

auto Lexer::readInputCharacter(uint8_t n) noexcept -> void {
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

auto Lexer::readOperator() const noexcept -> std::optional<Token> {
    const auto start_line = line_no_;
    const auto start_col  = col_no_;

    if (current_byte_ == '\0') {
        return Token{
            .type   = TokenType::END,
            .slice  = {},
            .line   = start_line,
            .column = start_col,
        };
    }

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
    if (max_len == 0) { return std::nullopt; }
    return Token{
        .type   = matched_type,
        .slice  = input_.substr(pos_, max_len),
        .line   = start_line,
        .column = start_col,
    };
}

auto Lexer::readIdent() noexcept -> std::string_view {
    const auto start = pos_;

    auto passed_first = false;
    while (std::isalpha(current_byte_) || current_byte_ == '_' ||
           (passed_first && std::isdigit(current_byte_))) {
        readInputCharacter();
        passed_first = true;
    }

    return input_.substr(start, pos_ - start);
}

enum class NumberSuffix {
    NONE     = 0,
    UNSIGNED = 1,
    SIZE     = 2,
};

auto Lexer::readNumber() noexcept -> Token {
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
            readInputCharacter(2);
        } else if (next == 'b' || next == 'B') {
            base = Base::BINARY;
            readInputCharacter(2);
        } else if (next == 'o' || next == 'O') {
            base = Base::OCTAL;
            readInputCharacter(2);
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
            readInputCharacter();

            if (current_byte_ == '+' || current_byte_ == '-') { readInputCharacter(); }
            while (std::isdigit(current_byte_)) {
                readInputCharacter();
            }

            continue;
        }

        // Fractional part
        if (base == Base::DECIMAL && c == '.') {
            if (peek_pos_ < input_.size() && input_[peek_pos_] == '.') { break; }
            if (passed_decimal) { break; }

            passed_decimal = true;
            readInputCharacter();
            continue;
        }

        // Normal digit
        if (digit_in_base(c, base)) {
            readInputCharacter();
            continue;
        }

        break;
    }

    // Quick non-base-10 length validation
    if (base != Base::DECIMAL && pos_ - start <= 2) {
        return {
            .type   = TokenType::ILLEGAL,
            .slice  = input_.substr(start, pos_ - start),
            .line   = start_line,
            .column = start_col,
        };
    }

    auto suffix{NumberSuffix::NONE};
    if (!(passed_decimal || passed_exponent) && pos_ < input_.size()) {
        auto c = current_byte_;
        if (c == 'u' || c == 'U') {
            suffix = NumberSuffix::UNSIGNED;
            readInputCharacter();

            c = current_byte_;
            if (c == 'z' || c == 'Z') {
                suffix = NumberSuffix::SIZE;
                readInputCharacter();
            }
        }
    }

    // Total validation
    const auto length = pos_ - start;
    auto       type{TokenType::ILLEGAL};
    if (length == 0) {
        return {
            .type   = type,
            .slice  = input_.substr(start, 1),
            .line   = start_line,
            .column = start_col,
        };
    }

    if (input_[pos_ - 1] == '.') {
        return {
            .type   = type,
            .slice  = input_.substr(start, length),
            .line   = start_line,
            .column = start_col,
        };
    }

    if (passed_decimal && (base != Base::DECIMAL)) {
        return {
            .type   = type,
            .slice  = input_.substr(start, length),
            .line   = start_line,
            .column = start_col,
        };
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

    return {
        .type   = type,
        .slice  = input_.substr(start, length),
        .line   = start_line,
        .column = start_col,
    };
}

auto Lexer::readEscape() noexcept -> byte {
    readInputCharacter();

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

auto Lexer::readString() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    readInputCharacter();

    while (current_byte_ != '"' && current_byte_ != '\0') {
        if (current_byte_ == '\\') { readEscape(); }
        readInputCharacter();
    }

    if (current_byte_ == '\0') {
        return {
            .type   = TokenType::ILLEGAL,
            .slice  = input_.substr(start, pos_ - start),
            .line   = start_line,
            .column = start_col,
        };
    }
    readInputCharacter();

    return {
        .type   = TokenType::STRING,
        .slice  = input_.substr(start, pos_ - start),
        .line   = start_line,
        .column = start_col,
    };
}

// Reads a multiline string from the token, assuming the '\\' operator has been consumed
auto Lexer::readMultilineString() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    auto       end        = start;

    while (true) {
        // Consume characters until newline or EOF
        while (current_byte_ != '\n' && current_byte_ != '\r' && current_byte_ != '\0') {
            readInputCharacter();
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
            end = pos_;
            break;
        }

        // Include the CRLF/LF newline in the token
        readInputCharacter();
        if (current_byte_ == '\r' && peek_pos_ < input_.size() && input_[peek_pos_] == '\n') {
            readInputCharacter();
        }

        // consume the next "\\" line continuation
        readInputCharacter(2);
    }

    return {
        .type   = TokenType::MULTILINE_STRING,
        .slice  = input_.substr(start, end - start),
        .line   = start_line,
        .column = start_col,
    };
}

// Reads a byte literal returning an illegal token for malformed literals.
//
// Assumes that the surrounding single quotes have not been consumed.
auto Lexer::readByteLiteral() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    readInputCharacter();

    // Consume one logical character
    if (current_byte_ == '\\') {
        readEscape();
        readInputCharacter();
    } else if (current_byte_ != '\'' && current_byte_ != '\n' && current_byte_ != '\r') {
        readInputCharacter();
    } else {
        return {
            .type   = TokenType::ILLEGAL,
            .slice  = input_.substr(start, pos_ - start),
            .line   = start_line,
            .column = start_col,
        };
    }

    // The next character MUST be closing ', otherwise illegally consume like a comment
    if (current_byte_ != '\'') {
        auto illegal_end = pos_;
        while (current_byte_ != '\'' && current_byte_ != '\n' && current_byte_ != '\r' &&
               current_byte_ != '\0') {
            readInputCharacter();
            illegal_end = pos_;
        }

        if (current_byte_ == '\'') {
            readInputCharacter();
            illegal_end = pos_;
        }

        return {
            .type   = TokenType::ILLEGAL,
            .slice  = input_.substr(start, illegal_end - start),
            .line   = start_line,
            .column = start_col,
        };
    }
    readInputCharacter();

    return {
        .type   = TokenType::CHARACTER,
        .slice  = input_.substr(start, pos_ - start),
        .line   = start_line,
        .column = start_col,
    };
}

// Reads a comment from the token, assuming the '//' operator has been consumed
auto Lexer::readComment() noexcept -> Token {
    const auto start      = pos_;
    const auto start_line = line_no_;
    const auto start_col  = col_no_;
    while (current_byte_ != '\n' && current_byte_ != '\0') {
        readInputCharacter();
    }

    return {
        .type   = TokenType::COMMENT,
        .slice  = input_.substr(start, pos_ - start),
        .line   = start_line,
        .column = start_col,
    };
}
