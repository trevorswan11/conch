#pragma once

#include <utility>
#include <vector>

#include <stdx/option.hh>
#include <stdx/result.hh>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "syntax/error.hh"

namespace ghoti {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

struct ExplicitArrayType {
    stdx::option<ExpressionHandle> dimension;
    bool                           null_terminated;
    ExplicitTypeID                 inner_explicit_type;
};

struct ExplicitFunctionType {
    std::vector<ExplicitTypeID> parameter_types;
    bool                        variadic;
    ExplicitTypeID              explicit_return_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExplicitFunctionType, syntax::Diagnostic>;
};

struct ExplicitType {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> stdx::result<ExplicitTypeID, syntax::Diagnostic>;

    // Parses an optionally present type and checks/advances for value initialization
    [[nodiscard]] static auto parse_opt_init(syntax::Parser& parser)
        -> stdx::result<std::pair<stdx::option<ExplicitTypeID>, bool>, syntax::Diagnostic>;
};

} // namespace ast

} // namespace ghoti
