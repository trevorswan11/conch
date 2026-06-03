#pragma once

// IWYU pragma: begin_exports

#include <fmt/base.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"

template <> struct fmt::formatter<ghoti::ast::TypeModifier> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const ghoti::ast::TypeModifier& t, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(t.underlying_));
    }
};

template <> struct fmt::formatter<ghoti::ast::IdentifierExpression> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const ghoti::ast::IdentifierExpression& n, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", n.name);
    }
};

template <ghoti::traits::ValuedPrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.value);
    }
};

// IWYU pragma: end_exports
