#pragma once

namespace conch::ast {

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
class SignedLongIntegerExpression;
class ISizeIntegerExpression;
class UnsignedIntegerExpression;
class UnsignedLongIntegerExpression;
class USizeIntegerExpression;
class ByteExpression;
class FloatExpression;
class BoolExpression;
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

class Visitor {
  public:
    virtual ~Visitor()                                               = default;
    virtual auto visit(const ArrayExpression&) -> void               = 0;
    virtual auto visit(const AssignmentExpression&) -> void          = 0;
    virtual auto visit(const BinaryExpression&) -> void              = 0;
    virtual auto visit(const CallExpression&) -> void                = 0;
    virtual auto visit(const DoWhileLoopExpression&) -> void         = 0;
    virtual auto visit(const DotExpression&) -> void                 = 0;
    virtual auto visit(const EnumExpression&) -> void                = 0;
    virtual auto visit(const ForLoopExpression&) -> void             = 0;
    virtual auto visit(const FunctionExpression&) -> void            = 0;
    virtual auto visit(const IdentifierExpression&) -> void          = 0;
    virtual auto visit(const IfExpression&) -> void                  = 0;
    virtual auto visit(const IndexExpression&) -> void               = 0;
    virtual auto visit(const InfiniteLoopExpression&) -> void        = 0;
    virtual auto visit(const MatchExpression&) -> void               = 0;
    virtual auto visit(const PrefixExpression&) -> void              = 0;
    virtual auto visit(const StringExpression&) -> void              = 0;
    virtual auto visit(const SignedIntegerExpression&) -> void       = 0;
    virtual auto visit(const SignedLongIntegerExpression&) -> void   = 0;
    virtual auto visit(const ISizeIntegerExpression&) -> void        = 0;
    virtual auto visit(const UnsignedIntegerExpression&) -> void     = 0;
    virtual auto visit(const UnsignedLongIntegerExpression&) -> void = 0;
    virtual auto visit(const USizeIntegerExpression&) -> void        = 0;
    virtual auto visit(const ByteExpression&) -> void                = 0;
    virtual auto visit(const FloatExpression&) -> void               = 0;
    virtual auto visit(const BoolExpression&) -> void                = 0;
    virtual auto visit(const RangeExpression&) -> void               = 0;
    virtual auto visit(const ScopeResolutionExpression&) -> void     = 0;
    virtual auto visit(const StructExpression&) -> void              = 0;
    virtual auto visit(const TypeExpression&) -> void                = 0;
    virtual auto visit(const WhileLoopExpression&) -> void           = 0;
    virtual auto visit(const BlockStatement&) -> void                = 0;
    virtual auto visit(const DeclStatement&) -> void                 = 0;
    virtual auto visit(const DiscardStatement&) -> void              = 0;
    virtual auto visit(const ExpressionStatement&) -> void           = 0;
    virtual auto visit(const ImportStatement&) -> void               = 0;
    virtual auto visit(const JumpStatement&) -> void                 = 0;
};

} // namespace conch::ast
