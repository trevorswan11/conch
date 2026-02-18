#pragma once

#include <string_view>

#include "optional.hpp"

#include "lexer/token.hpp"

namespace conch::tests {

auto test_ident(std::string_view input, Optional<TokenType> expected_type = TokenType::IDENT)
    -> void;

} // namespace conch::tests
