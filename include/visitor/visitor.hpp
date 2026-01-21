#pragma once

namespace ast {

class BlockStatement;

} // namespace ast

class Visitor {
  public:
    virtual ~Visitor()                            = default;
    virtual void visit(ast::BlockStatement& node) = 0;
};
