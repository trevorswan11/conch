#pragma once

#include <memory>

#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class GroupedExpression {
  public:
    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<Expression>, ParserDiagnostic>;
};

} // namespace ast
