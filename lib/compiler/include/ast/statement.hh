#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "ast/handle.hh"
#include "ast/id.hh"
#include "syntax/error.hh"
#include "syntax/token_type.hh"

#include <enum.hh>
#include <iterator.hh>
#include <option.hh>
#include <result.hh>
#include <types.hh>

namespace ghoti {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

struct BlockStatement {
    MAKE_ITERATOR(Statements, std::vector<StatementHandle>, statements);

    Statements statements;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct BreakStatement {
    opt::Option<IdentifierHandle> label;
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ContinueStatement {
    opt::Option<IdentifierHandle> label;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

enum class DeclModifiers : u8 {
    VARIABLE  = 1 << 0,
    CONSTANT  = 1 << 1,
    CONSTEXPR = 1 << 2,
    PUBLIC    = 1 << 3,
    EXTERN    = 1 << 4,
    EXPORT    = 1 << 5,
};

MAKE_ENUM_OPERATORS(DeclModifiers)

struct DeclStatement {
    IdentifierHandle              name;
    opt::Option<ExplicitTypeID>   explicit_type;
    opt::Option<ExpressionHandle> value;
    DeclModifiers                 modifiers;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;

    [[nodiscard]] auto has_modifier(DeclModifiers flag) const noexcept -> bool {
        return modifiers_has(modifiers, flag);
    }

  private:
    static auto modifiers_has(DeclModifiers modifiers, DeclModifiers flag) noexcept -> bool {
        return static_cast<bool>(modifiers & flag);
    }
};

struct DeferStatement {
    StatementHandle deferred;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct DiscardStatement {
    ExpressionHandle discarded;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ExpressionStatement {
    ExpressionHandle expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser, syntax::SemicolonBehavior behavior)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

class AST;

struct ImportStatement {
    ImportPayloadHandle           payload;
    opt::Option<IdentifierHandle> alias;

    [[nodiscard]] static constexpr auto is_public(ast::NodeID id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;

    [[nodiscard]] auto get_name(const AST& tree) const noexcept
        -> std::pair<ast::IdentifierHandle, std::string_view>;
};

struct ReturnStatement {
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct TestStatement {
    opt::Option<StringHandle> description;
    BlockHandle               block;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct UsingStatement {
    IdentifierHandle alias;
    ExplicitTypeID   explicit_type;

    [[nodiscard]] static constexpr auto is_public(ast::NodeID id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

} // namespace ast

} // namespace ghoti

template <> struct magic_enum::customize::enum_range<ghoti::ast::DeclModifiers> {
    static constexpr bool is_flags = true;
};
