#include "ast/id.hh"

#include <string_view>
#include <utility>

#include <stdx/enum.hh>
#include <stdx/fixed/enum_map.hh>

#include "ast/kind.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

namespace ghoti::ast {

namespace {

using NameMapping = std::pair<NodeKind, std::string_view>;

constexpr auto NODE_NAMES{stdx::fixed::EnumMap<NodeKind, std::string_view>::from(
    "expression",
    NameMapping{NodeKind::ARRAY_EXPRESSION, "array"},
    NameMapping{NodeKind::CALL_EXPRESSION, "call"},
    NameMapping{NodeKind::DO_WHILE_LOOP_EXPRESSION, "do-while loop"},
    NameMapping{NodeKind::ENUM_EXPRESSION, "enum"},
    NameMapping{NodeKind::FOR_LOOP_EXPRESSION, "for loop"},
    NameMapping{NodeKind::FUNCTION_EXPRESSION, "function"},
    NameMapping{NodeKind::IDENTIFIER_EXPRESSION, "identifier"},
    NameMapping{NodeKind::IF_EXPRESSION, "if"},
    NameMapping{NodeKind::INDEX_EXPRESSION, "index"},
    NameMapping{NodeKind::INFINITE_LOOP_EXPRESSION, "infinite loop"},
    NameMapping{NodeKind::ASSIGNMENT_EXPRESSION, "assignment"},
    NameMapping{NodeKind::BINARY_EXPRESSION, "binary"},
    NameMapping{NodeKind::DOT_EXPRESSION, "dot"},
    NameMapping{NodeKind::RANGE_EXPRESSION, "range"},
    NameMapping{NodeKind::INITIALIZER_EXPRESSION, "initializer"},
    NameMapping{NodeKind::LABEL_EXPRESSION, "label"},
    NameMapping{NodeKind::MATCH_EXPRESSION, "match"},
    NameMapping{NodeKind::UNARY_EXPRESSION, "unary"},
    NameMapping{NodeKind::REFERENCE_EXPRESSION, "reference-of"},
    NameMapping{NodeKind::ADDRESS_OF_EXPRESSION, "address-of"},
    NameMapping{NodeKind::DEREFERENCE_EXPRESSION, "dereference"},
    NameMapping{NodeKind::IMPLICIT_ACCESS_EXPRESSION, "implicit access"},
    NameMapping{NodeKind::STRING_EXPRESSION, "string"},
    NameMapping{NodeKind::I32_EXPRESSION, "i32"},
    NameMapping{NodeKind::I64_EXPRESSION, "i64"},
    NameMapping{NodeKind::ISIZE_EXPRESSION, "isize"},
    NameMapping{NodeKind::U32_EXPRESSION, "u32"},
    NameMapping{NodeKind::U64_EXPRESSION, "u64"},
    NameMapping{NodeKind::USIZE_EXPRESSION, "usize"},
    NameMapping{NodeKind::U8_EXPRESSION, "u8"},
    NameMapping{NodeKind::F32_EXPRESSION, "f32"},
    NameMapping{NodeKind::F64_EXPRESSION, "f64"},
    NameMapping{NodeKind::BOOL_EXPRESSION, "bool"},
    NameMapping{NodeKind::VOID_EXPRESSION, "void"},
    NameMapping{NodeKind::UNDEFINED_EXPRESSION, "undefined"},
    NameMapping{NodeKind::MODULE_ACCESS_EXPRESSION, "module access"},
    NameMapping{NodeKind::STRUCT_EXPRESSION, "struct"},
    NameMapping{NodeKind::UNION_EXPRESSION, "union"},
    NameMapping{NodeKind::WHILE_LOOP_EXPRESSION, "while loop"},
    NameMapping{NodeKind::BLOCK_STATEMENT, "statement"},
    NameMapping{NodeKind::BREAK_STATEMENT, "statement"},
    NameMapping{NodeKind::CONTINUE_STATEMENT, "statement"},
    NameMapping{NodeKind::DECL_STATEMENT, "statement"},
    NameMapping{NodeKind::DEFER_STATEMENT, "statement"},
    NameMapping{NodeKind::DISCARD_STATEMENT, "statement"},
    NameMapping{NodeKind::EXPRESSION_STATEMENT, "statement"},
    NameMapping{NodeKind::IMPORT_STATEMENT, "statement"},
    NameMapping{NodeKind::RETURN_STATEMENT, "statement"},
    NameMapping{NodeKind::TEST_STATEMENT, "statement"},
    NameMapping{NodeKind::USING_STATEMENT, "statement"})};

} // namespace

auto NodeID::display_name() const noexcept -> std::string_view { return NODE_NAMES[get_kind()]; }

namespace {

using TokenType       = syntax::TokenType;
using Modifier        = TypeModifier::Modifier;
using ModifierMapping = std::pair<TokenType, Modifier>;

constexpr auto MODIFIERS{stdx::fixed::EnumMap<TokenType, Modifier>::from(
    Modifier::VALUE,
    ModifierMapping{TokenType::BW_AND, Modifier::REF},
    ModifierMapping{TokenType::AND_MUT, Modifier::MUT_REF},
    ModifierMapping{TokenType::CARET, Modifier::PTR},
    ModifierMapping{TokenType::CARET_MUT, Modifier::MUT_PTR},
    ModifierMapping{TokenType::VOLATILE, Modifier::VOLATILE},
    ModifierMapping{TokenType::MUT_VOLATILE, Modifier::MUT_VOLATILE})};

} // namespace

TypeModifier::TypeModifier(const syntax::Token& tok) noexcept : underlying_{MODIFIERS[tok.type]} {}

} // namespace ghoti::ast
