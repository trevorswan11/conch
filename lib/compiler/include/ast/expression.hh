#pragma once

#include <string_view>
#include <vector>

#include <stdx/option.hh>
#include <stdx/result.hh>
#include <stdx/variant.hh>

#include "ast/handle.hh"
#include "ast/id.hh"
#include "syntax/error.hh"
#include "syntax/token_type.hh"

namespace ghoti {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

struct ArrayExpression {
    stdx::option<ExpressionHandle> size;
    bool                           null_terminated;
    ExplicitTypeID                 item_explicit_type;
    std::vector<ExpressionHandle>  items;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct CallExpression {
    using Argument = stdx::variant<ExpressionHandle, ExplicitTypeID>;

    ExpressionHandle      function;
    std::vector<Argument> arguments;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle function)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct DoWhileLoopExpression {
    BlockHandle      block;
    ExpressionHandle condition;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct EnumExpression {
    struct Enumeration {
        IdentifierHandle               name;
        stdx::option<ExpressionHandle> value;
    };

    stdx::option<IdentifierHandle> underlying;
    std::vector<Enumeration>       enumerations;
    bool                           non_exhaustive;
    Members                        members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct ForLoopExpression {
    struct Capture {
        TypeModifier           modifier;
        DiscardableIdentHandle payload;
    };

    std::vector<ExpressionHandle> iterables;
    std::vector<Capture>          captures;
    BlockHandle                   block;
    stdx::option<StatementHandle> non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct SelfParameter {
    TypeModifier     modifier;
    IdentifierHandle name;
};

} // namespace ast

} // namespace ghoti

namespace stdx {

template <> struct nullable<ghoti::ast::SelfParameter> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ghoti::ast::SelfParameter {
        return {.modifier = {}, .name = ghoti::ast::IdentifierHandle::make_invalid()};
    }

    [[nodiscard]] static constexpr auto is_valid(const ghoti::ast::SelfParameter& self) noexcept
        -> bool {
        return self.name.is_valid();
    }
};

} // namespace stdx

namespace ghoti::ast {

[[nodiscard]] auto try_parse_variadic_fn(syntax::Parser& parser)
    -> stdx::result<bool, syntax::Diagnostic>;

struct FunctionExpression {
    struct Parameter {
        IdentifierHandle name;
        ExplicitTypeID   explicit_type;
    };

    stdx::option<SelfParameter> self;
    std::vector<Parameter>      parameters;
    bool                        variadic;
    ExplicitTypeID              explicit_return_type;
    BlockHandle                 body;

    // Parse the function as a value. Meant for the parser LUT
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct GroupedExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct IdentifierExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;

    std::string_view name;
};

struct IfExpression {
    bool                          constexpr_condition;
    ExpressionHandle              condition;
    StatementHandle               consequence;
    stdx::option<StatementHandle> alternate;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct IndexExpression {
    ExpressionHandle array;
    ExpressionHandle index;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle array)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct InfiniteLoopExpression {
    BlockHandle block;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

#define DECLARE_INFIX_EXPRESSION(Type)                                                \
    struct Type {                                                                     \
        ExpressionHandle          lhs;                                                \
        ExpressionHandle          rhs;                                                \
        [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle lhs) \
            -> stdx::result<ExpressionHandle, syntax::Diagnostic>;                    \
    };

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(AssignmentExpression)

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(BinaryExpression)

struct DotExpression {
    OuterAccessHandle object;
    IdentifierHandle  member;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle outer)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(RangeExpression)

#undef DECLARE_INFIX_EXPRESSION

struct InitializerExpression {
    struct Initializer {
        ImplicitAccessHandle member;
        ExpressionHandle     value;
    };

    stdx::option<ExpressionHandle> object_type;
    std::vector<Initializer>       initializers;

    // Parse assuming an object is present. Meant for the parser LUT
    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle object)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
        return parse(parser, stdx::option<ExpressionHandle>{object});
    }

    // Parse the expression with a potentially empty object
    [[nodiscard]] static auto parse(syntax::Parser& parser, stdx::option<ExpressionHandle> object)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct LabelExpression {
    IdentifierHandle  name;
    LabeledNodeHandle body;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle name)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;

  private:
    [[nodiscard]] static auto deconstruct_body(syntax::Parser& parser, StatementHandle raw_stmt)
        -> stdx::result<LabeledNodeHandle, syntax::Diagnostic>;
};

struct MatchExpression {
    struct Arm {
        MatchPatternHandle                   pattern;
        stdx::option<DiscardableIdentHandle> capture;
        StatementHandle                      dispatch;
    };

    ExpressionHandle matcher;
    std::vector<Arm> arms;
    stdx::opt_size    catch_all_idx;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

#define DECLARE_PREFIX_EXPRESSION(Type)                            \
    struct Type {                                                  \
        ExpressionHandle          rhs;                             \
        [[nodiscard]] static auto parse(syntax::Parser& parser)    \
            -> stdx::result<ExpressionHandle, syntax::Diagnostic>; \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression)
DECLARE_PREFIX_EXPRESSION(AddressOfExpression)

struct ImplicitAccessExpression {
    IdentifierHandle member;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

#undef DECLARE_PREFIX_EXPRESSION

struct ModuleAccessExpression {
    OuterAccessHandle outer;
    IdentifierHandle  inner;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle outer)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct StructExpression {
    // Field publicity is baked into the identifier's token type
    struct Field {
        IdentifierHandle               name;
        ExplicitTypeID                 explicit_type;
        stdx::option<ExpressionHandle> default_value;

        [[nodiscard]] constexpr auto is_public() const noexcept -> bool {
            return name->get_token_type() == syntax::TokenType::PUBLIC;
        }
    };

    std::vector<Field> fields;
    Members            members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct UnionExpression {
    struct Field {
        IdentifierHandle name;
        ExplicitTypeID   explicit_type;
    };

    std::vector<Field> fields;
    Members            members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct WhileLoopExpression {
    ExpressionHandle               condition;
    stdx::option<ExpressionHandle> continuation;
    BlockHandle                    block;
    stdx::option<StatementHandle>  non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

} // namespace ghoti::ast
