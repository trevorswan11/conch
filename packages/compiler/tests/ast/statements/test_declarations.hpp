#pragma once

#include <string_view>

#include "ast/statements/decl.hpp"

namespace conch::tests::helpers {

auto test_decl(std::string_view input, const ast::DeclStatement& expected) -> void;

} // namespace conch::tests::helpers
