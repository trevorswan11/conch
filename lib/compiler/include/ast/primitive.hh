#pragma once

#include <string>
#include <type_traits>

#include <stdx/result.hh>
#include <stdx/types.hh>

#include "ast/handle.hh"
#include "syntax/error.hh"

namespace ghoti {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

#define DECLARE_PRIMITIVE_EXPRESSION(NodeType, ValueType)          \
    struct NodeType {                                              \
        using value_type = ValueType;                              \
        value_type                value;                           \
        [[nodiscard]] static auto parse(syntax::Parser& parser)    \
            -> stdx::result<ExpressionHandle, syntax::Diagnostic>; \
    };

DECLARE_PRIMITIVE_EXPRESSION(StringExpression, std::string)
DECLARE_PRIMITIVE_EXPRESSION(I32Expression, i32)
DECLARE_PRIMITIVE_EXPRESSION(I64Expression, i64)
DECLARE_PRIMITIVE_EXPRESSION(ISizeExpression, isize)
DECLARE_PRIMITIVE_EXPRESSION(U32Expression, u32)
DECLARE_PRIMITIVE_EXPRESSION(U64Expression, u64)
DECLARE_PRIMITIVE_EXPRESSION(USizeExpression, usize)
DECLARE_PRIMITIVE_EXPRESSION(U8Expression, u8)
DECLARE_PRIMITIVE_EXPRESSION(F32Expression, f32)
DECLARE_PRIMITIVE_EXPRESSION(F64Expression, f64)

#undef DECLARE_PRIMITIVE_EXPRESSION

} // namespace ast

namespace traits {

template <typename T> struct is_valued_primitive : std::false_type {};
template <> struct is_valued_primitive<ast::StringExpression> : std::true_type {};
template <> struct is_valued_primitive<ast::I32Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::I64Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::ISizeExpression> : std::true_type {};
template <> struct is_valued_primitive<ast::U32Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::U64Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::USizeExpression> : std::true_type {};
template <> struct is_valued_primitive<ast::U8Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::F32Expression> : std::true_type {};
template <> struct is_valued_primitive<ast::F64Expression> : std::true_type {};

// A primitive node with its value embedded in the data
template <typename T>
concept ValuedPrimitive = is_valued_primitive<T>::value;

} // namespace traits

namespace ast {

struct BoolExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct VoidExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

struct UndefinedExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExpressionHandle, syntax::Diagnostic>;
};

} // namespace ast

} // namespace ghoti
