#include "ast/id.hh"

#include <string_view>
#include <utility>

#include "ast/kind.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include <enum.hh>
#include <fixed/enum_map.hh>

namespace ghoti::ast {

namespace {

constexpr auto NODE_NAMES = [] {
    fixed::EnumMap<NodeKind, std::string_view> names{"expression"};

    names[NodeKind::ARRAY_EXPRESSION]           = "array";
    names[NodeKind::CALL_EXPRESSION]            = "call";
    names[NodeKind::DO_WHILE_LOOP_EXPRESSION]   = "do-while loop";
    names[NodeKind::ENUM_EXPRESSION]            = "enum";
    names[NodeKind::FOR_LOOP_EXPRESSION]        = "for loop";
    names[NodeKind::FUNCTION_EXPRESSION]        = "function";
    names[NodeKind::IDENTIFIER_EXPRESSION]      = "identifier";
    names[NodeKind::IF_EXPRESSION]              = "if";
    names[NodeKind::INDEX_EXPRESSION]           = "index";
    names[NodeKind::INFINITE_LOOP_EXPRESSION]   = "infinite loop";
    names[NodeKind::ASSIGNMENT_EXPRESSION]      = "assignment";
    names[NodeKind::BINARY_EXPRESSION]          = "binary";
    names[NodeKind::DOT_EXPRESSION]             = "dot";
    names[NodeKind::RANGE_EXPRESSION]           = "range";
    names[NodeKind::INITIALIZER_EXPRESSION]     = "initializer";
    names[NodeKind::LABEL_EXPRESSION]           = "label";
    names[NodeKind::MATCH_EXPRESSION]           = "match";
    names[NodeKind::UNARY_EXPRESSION]           = "unary";
    names[NodeKind::REFERENCE_EXPRESSION]       = "reference-of";
    names[NodeKind::ADDRESS_OF_EXPRESSION]      = "address-of";
    names[NodeKind::DEREFERENCE_EXPRESSION]     = "dereference";
    names[NodeKind::IMPLICIT_ACCESS_EXPRESSION] = "implicit access";
    names[NodeKind::STRING_EXPRESSION]          = "string";
    names[NodeKind::I32_EXPRESSION]             = "i32";
    names[NodeKind::I64_EXPRESSION]             = "i64";
    names[NodeKind::ISIZE_EXPRESSION]           = "isize";
    names[NodeKind::U32_EXPRESSION]             = "u32";
    names[NodeKind::U64_EXPRESSION]             = "u64";
    names[NodeKind::USIZE_EXPRESSION]           = "usize";
    names[NodeKind::U8_EXPRESSION]              = "u8";
    names[NodeKind::F32_EXPRESSION]             = "f32";
    names[NodeKind::F64_EXPRESSION]             = "f64";
    names[NodeKind::BOOL_EXPRESSION]            = "bool";
    names[NodeKind::VOID_EXPRESSION]            = "void";
    names[NodeKind::UNDEFINED_EXPRESSION]       = "undefined";
    names[NodeKind::MODULE_ACCESS_EXPRESSION]   = "module access";
    names[NodeKind::STRUCT_EXPRESSION]          = "struct";
    names[NodeKind::UNION_EXPRESSION]           = "union";
    names[NodeKind::WHILE_LOOP_EXPRESSION]      = "while loop";

    for (const auto kind : enum_range<NodeKind::BLOCK_STATEMENT, NodeKind::USING_STATEMENT>()) {
        names[kind] = "statement";
    }
    return names;
}();

} // namespace

auto NodeID::display_name() const noexcept -> std::string_view { return NODE_NAMES[get_kind()]; }

namespace {

using Modifier           = TypeModifier::Modifier;
using ModifierMapping    = std::pair<syntax::TokenType, Modifier>;
constexpr auto MODIFIERS = [] {
    using TokenType = syntax::TokenType;
    fixed::EnumMap<TokenType, Modifier> modifiers{Modifier::VALUE};
    modifiers[TokenType::BW_AND]       = Modifier::REF;
    modifiers[TokenType::AND_MUT]      = Modifier::MUT_REF;
    modifiers[TokenType::CARET]        = Modifier::PTR;
    modifiers[TokenType::CARET_MUT]    = Modifier::MUT_PTR;
    modifiers[TokenType::VOLATILE]     = Modifier::VOLATILE;
    modifiers[TokenType::MUT_VOLATILE] = Modifier::MUT_VOLATILE;
    return modifiers;
}();

} // namespace

TypeModifier::TypeModifier(const syntax::Token& tok) noexcept : underlying_{MODIFIERS[tok.type]} {}

} // namespace ghoti::ast
