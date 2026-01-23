#include "ast/statements/jump.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto JumpStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<JumpStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
