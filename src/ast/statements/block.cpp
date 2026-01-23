#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<BlockStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
