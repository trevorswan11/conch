#pragma once

namespace conch {
namespace ast {

class ArrayExpression;
class AssignmentExpression;
class BinaryExpression;
class CallExpression;
class DoWhileLoopExpression;
class DotExpression;
class EnumExpression;
class ForLoopExpression;
class FunctionExpression;
class IdentifierExpression;
class IfExpression;
class IndexExpression;
class InfiniteLoopExpression;
class MatchExpression;
class PrefixExpression;
class StringExpression;
class SignedIntegerExpression;
class UnsignedIntegerExpression;
class SizeIntegerExpression;
class ByteExpression;
class FloatExpression;
class BoolExpression;
class VoidExpression;
class NilExpression;
class RangeExpression;
class ScopeResolutionExpression;
class StructExpression;
class TypeExpression;
class WhileLoopExpression;

class BlockStatement;
class DeclStatement;
class DiscardStatement;
class ExpressionStatement;
class ImportStatement;
class JumpStatement;
class NamespaceStatement;

} // namespace ast

class Visitor {
  public:
    virtual ~Visitor()                                                = default;
    virtual auto visit(const ast::ArrayExpression&) -> void           = 0;
    virtual auto visit(const ast::AssignmentExpression&) -> void      = 0;
    virtual auto visit(const ast::BinaryExpression&) -> void          = 0;
    virtual auto visit(const ast::CallExpression&) -> void            = 0;
    virtual auto visit(const ast::DoWhileLoopExpression&) -> void     = 0;
    virtual auto visit(const ast::DotExpression&) -> void             = 0;
    virtual auto visit(const ast::EnumExpression&) -> void            = 0;
    virtual auto visit(const ast::ForLoopExpression&) -> void         = 0;
    virtual auto visit(const ast::FunctionExpression&) -> void        = 0;
    virtual auto visit(const ast::IdentifierExpression&) -> void      = 0;
    virtual auto visit(const ast::IfExpression&) -> void              = 0;
    virtual auto visit(const ast::IndexExpression&) -> void           = 0;
    virtual auto visit(const ast::InfiniteLoopExpression&) -> void    = 0;
    virtual auto visit(const ast::MatchExpression&) -> void           = 0;
    virtual auto visit(const ast::PrefixExpression&) -> void          = 0;
    virtual auto visit(const ast::StringExpression&) -> void          = 0;
    virtual auto visit(const ast::SignedIntegerExpression&) -> void   = 0;
    virtual auto visit(const ast::UnsignedIntegerExpression&) -> void = 0;
    virtual auto visit(const ast::SizeIntegerExpression&) -> void     = 0;
    virtual auto visit(const ast::ByteExpression&) -> void            = 0;
    virtual auto visit(const ast::FloatExpression&) -> void           = 0;
    virtual auto visit(const ast::BoolExpression&) -> void            = 0;
    virtual auto visit(const ast::VoidExpression&) -> void            = 0;
    virtual auto visit(const ast::NilExpression&) -> void             = 0;
    virtual auto visit(const ast::RangeExpression&) -> void           = 0;
    virtual auto visit(const ast::ScopeResolutionExpression&) -> void = 0;
    virtual auto visit(const ast::StructExpression&) -> void          = 0;
    virtual auto visit(const ast::TypeExpression&) -> void            = 0;
    virtual auto visit(const ast::WhileLoopExpression&) -> void       = 0;
    virtual auto visit(const ast::BlockStatement&) -> void            = 0;
    virtual auto visit(const ast::DeclStatement&) -> void             = 0;
    virtual auto visit(const ast::DiscardStatement&) -> void          = 0;
    virtual auto visit(const ast::ExpressionStatement&) -> void       = 0;
    virtual auto visit(const ast::ImportStatement&) -> void           = 0;
    virtual auto visit(const ast::JumpStatement&) -> void             = 0;
    virtual auto visit(const ast::NamespaceStatement&) -> void        = 0;
};

} // namespace conch
