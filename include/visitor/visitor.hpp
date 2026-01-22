#pragma once

namespace ast {

class ScopeResolutionExpression;
class IfExpression;
class AssignmentExpression;
class StructExpression;
class ForLoopExpression;
class WhileLoopExpression;
class DoWhileLoopExpression;
class InfiniteLoopExpression;
class EnumExpression;
class IdentifierExpression;
class StringExpression;
class SignedIntegerExpression;
class UnsignedIntegerExpression;
class SizeIntegerExpression;
class ByteExpression;
class BoolExpression;
class FloatExpression;
class VoidExpression;
class NilExpression;
class ArrayExpression;
class InfixExpression;
class TypeExpression;
class PrefixExpression;
class FunctionExpression;
class MatchExpression;
class DeclStatement;
class ImportStatement;
class BlockStatement;
class ImplStatement;
class JumpStatement;
class ExpressionStatement;
class DiscardStatement;
class IndexExpression;
class CallExpression;

} // namespace ast

class Visitor {
  public:
    virtual ~Visitor()                                                = default;
    virtual auto visit(const ast::ScopeResolutionExpression&) -> void = 0;
    virtual auto visit(const ast::IfExpression&) -> void              = 0;
    virtual auto visit(const ast::AssignmentExpression&) -> void      = 0;
    virtual auto visit(const ast::StructExpression&) -> void          = 0;
    virtual auto visit(const ast::ForLoopExpression&) -> void         = 0;
    virtual auto visit(const ast::WhileLoopExpression&) -> void       = 0;
    virtual auto visit(const ast::DoWhileLoopExpression&) -> void     = 0;
    virtual auto visit(const ast::InfiniteLoopExpression&) -> void    = 0;
    virtual auto visit(const ast::EnumExpression&) -> void            = 0;
    virtual auto visit(const ast::IdentifierExpression&) -> void      = 0;
    virtual auto visit(const ast::StringExpression&) -> void          = 0;
    virtual auto visit(const ast::SignedIntegerExpression&) -> void   = 0;
    virtual auto visit(const ast::UnsignedIntegerExpression&) -> void = 0;
    virtual auto visit(const ast::SizeIntegerExpression&) -> void     = 0;
    virtual auto visit(const ast::ByteExpression&) -> void            = 0;
    virtual auto visit(const ast::BoolExpression&) -> void            = 0;
    virtual auto visit(const ast::FloatExpression&) -> void           = 0;
    virtual auto visit(const ast::VoidExpression&) -> void            = 0;
    virtual auto visit(const ast::NilExpression&) -> void             = 0;
    virtual auto visit(const ast::ArrayExpression&) -> void           = 0;
    virtual auto visit(const ast::InfixExpression&) -> void           = 0;
    virtual auto visit(const ast::TypeExpression&) -> void            = 0;
    virtual auto visit(const ast::PrefixExpression&) -> void          = 0;
    virtual auto visit(const ast::FunctionExpression&) -> void        = 0;
    virtual auto visit(const ast::MatchExpression&) -> void           = 0;
    virtual auto visit(const ast::DeclStatement&) -> void             = 0;
    virtual auto visit(const ast::ImportStatement&) -> void           = 0;
    virtual auto visit(const ast::BlockStatement&) -> void            = 0;
    virtual auto visit(const ast::ImplStatement&) -> void             = 0;
    virtual auto visit(const ast::JumpStatement&) -> void             = 0;
    virtual auto visit(const ast::ExpressionStatement&) -> void       = 0;
    virtual auto visit(const ast::DiscardStatement&) -> void          = 0;
    virtual auto visit(const ast::IndexExpression&) -> void           = 0;
    virtual auto visit(const ast::CallExpression&) -> void            = 0;
};
