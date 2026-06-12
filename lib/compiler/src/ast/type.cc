#include "ast/type.hh"

#include <utility>
#include <vector>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "syntax/error.hh"
#include "syntax/parser.hh"
#include "syntax/precedence.hh"
#include "syntax/token_type.hh"

#include <option.hh>
#include <profiler.hh>
#include <result.hh>

namespace ghoti::ast {

auto ExplicitFunctionType::parse(syntax::Parser& parser)
    -> Result<ExplicitFunctionType, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    const auto start_token{parser.get_current_token()};
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    std::vector<ExplicitTypeID> parameter_types;
    bool                        variadic{false};
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else {
        // There is no self parameter for types
        while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            if (TRY(try_parse_variadic_fn(parser))) {
                variadic = true;
                break;
            }

            // There are no default values for parameters, and they must be explicitly typed
            auto type{TRY(ExplicitType::parse(parser))};
            if (type.is<IdentifierExpression>()) {
                // noreturn is not allowed for parameters
                if (type.get_token_type() == syntax::TokenType::NORETURN) {
                    return make_syntax_err("Function parameter types may not be marked `noreturn`",
                                           syntax::Error::FN_PARAMETER_IS_NORETURN,
                                           parser.get_location_of(type));
                }
            }

            parameter_types.emplace_back(type);
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
        }
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    }

    // There must be a return type but there cannot be a block
    TRY(parser.expect_peek(syntax::TokenType::COLON));
    const auto return_type{TRY(ExplicitType::parse(parser))};
    if (parser.peek_token_is(syntax::TokenType::LBRACE)) {
        return make_syntax_err("Function types may not have a body",
                               syntax::Error::EXPLICIT_FN_TYPE_HAS_BODY,
                               start_token);
    }

    return ExplicitFunctionType{.parameter_types      = std::move(parameter_types),
                                .variadic             = variadic,
                                .explicit_return_type = return_type};
}

auto ExplicitType::parse(syntax::Parser& parser) -> Result<ExplicitTypeID, syntax::Diagnostic> {
    // Always check for a modifier and advance past it if present
    PROFILE_FUNCTION();
    const auto         modifier_token{parser.get_peek_token()};
    const TypeModifier modifier{modifier_token};
    if (!modifier.is_value()) { parser.advance(); }

    // The array dimension of a type are only present conditionally
    if (parser.peek_token_is(syntax::TokenType::LBRACKET)) {
        parser.advance();

        auto                          null_terminated{false};
        opt::Option<ExpressionHandle> dimension;
        if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
            parser.advance();
            null_terminated = true;
        } else if (!parser.peek_token_is(syntax::TokenType::RBRACKET)) {
            parser.advance();
            dimension.emplace(TRY(parser.parse_expression()));

            // The null terminated marker comes after the size for explicitly sized types
            if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
                parser.advance();
                null_terminated = true;
            }
        }
        TRY(parser.expect_peek(syntax::TokenType::RBRACKET));

        // Arrays are recursively defined
        const auto inner{TRY(ExplicitType::parse(parser))};
        return parser.add_type<ExplicitArrayType>(
            modifier_token, modifier, dimension, null_terminated, inner);
    }

    if (!TypeModifier{parser.get_peek_token()}.is_value()) {
        // Don't advance since the parser does it implicitly here (costs two modifier queries)
        const auto inner{TRY(ExplicitType::parse(parser))};
        return parser.add_type<ExplicitTypeID>(modifier_token, modifier, inner);
    }

    // Otherwise the type has to be a 'simple' function or ident
    const auto& peek_token{parser.get_peek_token()};
    if (peek_token.is_valid_ident()) {
        // It's trivial to catch these syntactic errors here
        if (!modifier.is_value()) {
            switch (peek_token.type) {
            case syntax::TokenType::TYPE_TYPE:
                return make_syntax_err("Explicit `type` type cannot have a modifier",
                                       syntax::Error::ILLEGAL_TYPE_TYPE_MODIFIER,
                                       modifier_token);
            case syntax::TokenType::VOID_TYPE:
                return make_syntax_err("Explicit `void` type cannot have a modifier",
                                       syntax::Error::ILLEGAL_VOID_TYPE_MODIFIER,
                                       modifier_token);
            case syntax::TokenType::NORETURN:
                return make_syntax_err("Explicit `noreturn` type cannot have a modifier",
                                       syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                       modifier_token);
            default: break;
            }
        }

        parser.advance();
        // Manually dispatch to prevent weird consumption
        if (parser.peek_token_is(syntax::TokenType::COLON_COLON) ||
            parser.peek_token_is(syntax::TokenType::LPAREN)) {
            const auto parsed{TRY(parser.parse_expression(syntax::Precedence::TYPE))};
            if (parsed.is<ModuleAccessExpression>()) {
                return parser.add_type<ModuleAccessExpression>(
                    modifier_token, modifier, parser.get_node<ModuleAccessExpression>(*parsed));
            }

            if (parsed.is<DotExpression>()) {
                return parser.add_type<DotExpression>(
                    modifier_token, modifier, parser.get_node<DotExpression>(*parsed));
            }

            if (parsed.is<CallExpression>()) {
                return parser.add_type<CallExpression>(
                    modifier_token, modifier, parser.get_node<CallExpression>(*parsed));
            }

            return make_syntax_err(syntax::Error::ILLEGAL_EXPLICIT_TYPE,
                                   parser.get_location_of(*parsed));
        }

        const auto ident{TRY(IdentifierExpression::parse(parser))};
        return parser.add_type<IdentifierExpression>(
            modifier_token, modifier, parser.get_node<IdentifierExpression>(*ident));
    }

    // The inner type is limited to functions and user-defined types
    const auto type_start{parser.get_current_token()};
    if (parser.peek_token_is(syntax::TokenType::FUNCTION)) {
        parser.advance();
        const auto fn_type{TRY(ExplicitFunctionType::parse(parser))};
        if (!(modifier.is_value() || modifier.is_ptr())) {
            return make_syntax_err("Functions types may only be values or pointers",
                                   syntax::Error::ILLEGAL_FUNCTION_TYPE_MODIFIER,
                                   type_start);
        }

        // Function types cannot have bodies
        return parser.add_type<ExplicitFunctionType>(modifier_token, modifier, fn_type);
    }

    // The user-defined types can be handled by parsing any expression and verifying it
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::MISSING_EXPLICIT_TYPE, type_start);
    }

    opt::Option<ExplicitTypeID> id;
    switch (const auto user{TRY(parser.parse_expression())}; user->get_kind()) {
    case NodeKind::STRUCT_EXPRESSION:
        id.emplace(parser.add_type<StructExpression>(
            modifier_token, modifier, parser.get_node<StructExpression>(*user)));
        break;
    case NodeKind::ENUM_EXPRESSION:
        id.emplace(parser.add_type<EnumExpression>(
            modifier_token, modifier, parser.get_node<EnumExpression>(*user)));
        break;
    case NodeKind::UNION_EXPRESSION:
        id.emplace(parser.add_type<UnionExpression>(
            modifier_token, modifier, parser.get_node<UnionExpression>(*user)));
        break;
    default: break;
    }

    if (id) {
        if (!modifier.is_value()) {
            return make_syntax_err(
                "User-defined types can only be defined with non-modified aliases",
                syntax::Error::ILLEGAL_USING_ALIAS_WITH_MODIFIERS,
                type_start);
        }
        return *id;
    }
    return make_syntax_err(syntax::Error::ILLEGAL_EXPLICIT_TYPE, type_start);
}

namespace {

// Parses the explicit type if present and checks for an upcoming assignment for init
[[nodiscard]] auto parse_type_and_initializer(syntax::Parser& parser)
    -> Result<std::pair<opt::Option<ExplicitTypeID>, bool>, syntax::Diagnostic> {
    if (parser.peek_token_is(syntax::TokenType::WALRUS)) {
        parser.advance();
        return std::pair{opt::none, true};
    }

    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        const auto explicit_type{TRY(ExplicitType::parse(parser))};
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance();
            return std::pair{explicit_type, true};
        }
        return std::pair{explicit_type, false};
    }

    return Err{parser.peek_error(syntax::TokenType::COLON)};
}

} // namespace

auto ExplicitType::parse_opt_init(syntax::Parser& parser)
    -> Result<std::pair<opt::Option<ExplicitTypeID>, bool>, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    const auto [type, initialized] = TRY(parse_type_and_initializer(parser));

    // Advance again to prepare for rhs
    if (initialized) { parser.advance(); }
    return std::pair{type, initialized};
}

} // namespace ghoti::ast
