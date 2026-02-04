#include <catch_amalgamated.hpp>

#include "ast_nodes.hpp"

using namespace conch;

TEST_CASE("Integer parsing") { helpers::numbers("0", TokenType::INT_10, 0); }
