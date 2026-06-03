#pragma once

#include <utility>
#include <vector>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "syntax/error.hh"

#include "option.hh"
#include "result.hh"

namespace ghoti {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

struct ExplicitArrayType {
    opt::Option<ExpressionHandle> dimension;
    bool                          null_terminated;
    ExplicitTypeID                inner_explicit_type;
};

struct ExplicitFunctionType {
    std::vector<ExplicitTypeID> parameter_types;
    bool                        variadic;
    ExplicitTypeID              explicit_return_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExplicitFunctionType, syntax::Diagnostic>;
};

struct ExplicitType {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExplicitTypeID, syntax::Diagnostic>;

    // Parses an optionally present type and checks/advances for value initialization
    [[nodiscard]] static auto parse_opt_init(syntax::Parser& parser)
        -> Result<std::pair<opt::Option<ExplicitTypeID>, bool>, syntax::Diagnostic>;
};

} // namespace ast

} // namespace ghoti
