#include "ast/dumper.hh"

#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/type.hh"
#include "syntax/builtins.hh"
#include "syntax/token_type.hh"

#include "indent.hh"
#include "variant.hh"

namespace porpoise::ast {

auto ASTDumper::visit(NodeID, const ArrayExpression& array) -> void {
    fmt::println(out_, "ArrayExpression");
    {
        const Indent::Guard g{indent_, false};
        if (array.size) {
            fmt::print(out_, "{}Size: ", indent_.current_branch());
            dump(*array.size);
        } else {
            fmt::println(out_, "{}Size: (inferred)", indent_.current_branch());
        }
    }

    {
        const Indent::Guard g_inner{indent_, false};
        fmt::println(
            out_, "{}Null terminated: {}", indent_.current_branch(), array.null_terminated);
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        dump(array.item_explicit_type);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Items:", indent_.current_branch());
        dump_node_list(array.items);
    }
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const CallExpression& call) -> void {
    fmt::println(out_, "CallExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Callee: ", indent_.current_branch());
        dump(call.function);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Arguments:", indent_.current_branch());
        dump_container(call.arguments, [this](const CallExpression::Argument& arg) {
            fmt::print(out_, "{}", indent_.current_branch());
            std::visit([this](auto arg_id) { dump(arg_id); }, arg.id);
        });
    }
}

auto ASTDumper::visit(NodeID, const DoWhileLoopExpression& do_while) -> void {
    fmt::println(out_, "DoWhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        dump(do_while.block);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        dump(do_while.condition);
    }
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const EnumExpression& enum_expr) -> void {
    fmt::println(out_, "EnumExpression");

    if (enum_expr.underlying) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Underlying: ", indent_.current_branch());
        dump(*enum_expr.underlying);
    }

    if (!enum_expr.enumerations.empty()) {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Enumerations:", indent_.current_branch());
        dump_container(enum_expr.enumerations,
                       [this](const EnumExpression::Enumeration& enumeration) {
                           {
                               fmt::print(out_, "{}Name: ", indent_.current_branch());
                               const Indent::Guard g_name{indent_, !enumeration.value};
                               dump(enumeration.name);
                           }

                           if (enumeration.value) {
                               const Indent::Guard g_val{indent_, true};
                               fmt::print(out_, "{}Default: ", indent_.current_branch());
                               dump(*enumeration.value);
                           }
                       });
    }

    const auto has_members = !enum_expr.members.empty();
    {
        const Indent::Guard g{indent_, !has_members};
        fmt::println(
            out_, "{}Non-Exhaustive: {}", indent_.current_branch(), enum_expr.non_exhaustive);
    }

    if (has_members) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Members:", indent_.current_branch());
        dump_node_list(enum_expr.members);
    }
}

auto ASTDumper::visit(NodeID, const ForLoopExpression& for_loop) -> void {
    fmt::println(out_, "ForLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Iterables:", indent_.current_branch());
        dump_node_list(for_loop.iterables);
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Captures:", indent_.current_branch());
        dump_container(for_loop.captures, [this](const ForLoopExpression::Capture& capture) {
            fmt::print(out_, "{}", indent_.current_branch());
            if (capture.payload.is<Unit>()) {
                fmt::println(out_, "<discarded>");
            } else {
                const auto& ident = ast_.get_as<IdentifierExpression>(*capture.payload);
                fmt::println(out_, "{} (modifier: {})", ident, capture.modifier);
            }
        });
    }

    const auto has_non_break = for_loop.non_break.has_value();
    {
        const Indent::Guard g{indent_, !has_non_break};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        dump(for_loop.block);
    }

    if (has_non_break) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break: ", indent_.current_branch());
        dump(*for_loop.non_break);
    }
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const FunctionExpression& function) -> void {
    fmt::println(out_, "FunctionExpression");
    if (function.self) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}", indent_.current_branch());
        const auto& ident = ast_.get_as<IdentifierExpression>(*function.self->name);
        fmt::println(out_, "Self: {} (modifier: {})", ident, function.self->modifier);
    }

    if (!function.parameters.empty()) {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Parameters:", indent_.current_branch());
        dump_container(function.parameters, [this](const FunctionExpression::Parameter& parameter) {
            fmt::println(out_, "{}Param:", indent_.current_branch());
            {
                const Indent::Guard g_name{indent_, false};
                fmt::print(out_, "{}Name: ", indent_.current_branch());
                dump(*parameter.name);
            }

            {
                const Indent::Guard g_type{indent_, true};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump(parameter.explicit_type);
            }
        });
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Variadic: {}", indent_.current_branch(), function.variadic);
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Returns: ", indent_.current_branch());
        dump(function.explicit_return_type);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        dump(*function.body);
    }
}

auto ASTDumper::visit(NodeID id, const IdentifierExpression& ident) -> void {
    fmt::print(out_, "IdentifierExpression: {}", ident);
    if (syntax::get_builtin_opt(id.get_token_type())) {
        fmt::print(out_, " (builtin)");
    } else if (syntax::token_type::is_primitive(id.get_token_type())) {
        fmt::print(out_, " (primitive)");
    }
    fmt::println(out_, "");
}

auto ASTDumper::visit(NodeID, const IfExpression& if_expr) -> void {
    fmt::println(out_, "IfExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(
            out_, "{}Constexpr: {}", indent_.current_branch(), if_expr.constexpr_condition);
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        dump(if_expr.condition);
    }

    const auto has_alternate = if_expr.alternate.has_value();
    {
        Indent::Guard g{indent_, !has_alternate};
        fmt::print(out_, "{}Consequence: ", indent_.current_branch());
        dump(if_expr.consequence);
    }

    if (has_alternate) {
        Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alternate: ", indent_.current_branch());
        dump(*if_expr.alternate);
    }
}

auto ASTDumper::visit(NodeID, const IndexExpression& index) -> void {
    fmt::println(out_, "IndexExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Object: ", indent_.current_branch());
        dump(index.array);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Index: ", indent_.current_branch());
        dump(index.index);
    }
}

auto ASTDumper::visit(NodeID, const InfiniteLoopExpression& loop) -> void {
    fmt::println(out_, "InfiniteLoopExpression");
    const auto& block = ast_.get_as<BlockStatement>(*loop.block);
    dump_node_list(block);
}

#define MAKE_INFIX_DUMP(NodeType, LeftLabel, RightLabel)                                   \
    auto ASTDumper::visit(NodeID id, const NodeType& node) -> void {                       \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(id.get_token_type())); \
        {                                                                                  \
            const Indent::Guard g{indent_, false};                                         \
            fmt::print(out_, "{}" #LeftLabel ": ", indent_.current_branch());              \
            dump(node.lhs);                                                                \
        }                                                                                  \
        {                                                                                  \
            const Indent::Guard g{indent_, true};                                          \
            fmt::print(out_, "{}" #RightLabel ": ", indent_.current_branch());             \
            dump(node.rhs);                                                                \
        }                                                                                  \
    }

MAKE_INFIX_DUMP(AssignmentExpression, Assignee, Value)
MAKE_INFIX_DUMP(BinaryExpression, LHS, RHS)

auto ASTDumper::visit(NodeID id, const DotExpression& node) -> void {
    fmt::println(out_, "DotExpression ({})", magic_enum::enum_name(id.get_token_type()));
    {
        const Indent::Guard g{indent_, false};
        fmt ::print(out_, "{}Object: ", indent_.current_branch());
        dump(node.object);
    }
    {
        const Indent ::Guard g{indent_, true};
        fmt ::print(out_, "{}Member: ", indent_.current_branch());
        dump(node.member);
    }
}

MAKE_INFIX_DUMP(RangeExpression, Lower, Upper)

auto ASTDumper::visit(NodeID, const InitializerExpression& init) -> void {
    fmt::println(out_, "Initializer Expression:");
    const auto has_initializers = !init.initializers.empty();
    {
        const Indent::Guard g{indent_, !has_initializers};
        fmt::print(out_, "{}Object Type: ", indent_.current_branch());
        if (init.object_type) {
            dump(*init.object_type);
        } else {
            fmt::println(out_, "<inferred>");
        }
    }

    if (has_initializers) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Initializers:", indent_.current_branch());
        dump_container(init.initializers,
                       [this](const InitializerExpression::Initializer& initializer) {
                           fmt::println(out_, "{}Initializer:", indent_.current_branch());
                           {
                               const Indent::Guard g_inner{indent_, false};
                               fmt::print(out_, "{}Member: ", indent_.current_branch());
                               dump(initializer.member);
                           }

                           {
                               const Indent::Guard g_inner{indent_, true};
                               fmt::print(out_, "{}Value: ", indent_.current_branch());
                               dump(initializer.value);
                           }
                       });
    }
}

auto ASTDumper::visit(NodeID, const LabelExpression& label) -> void {
    fmt::println(out_, "Label Expression:");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Label: ", indent_.current_branch());
        dump(label.name);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        dump(label.body);
    }
}

auto ASTDumper::visit(NodeID, const MatchExpression& match) -> void {
    fmt::println(out_, "MatchExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Matcher: ", indent_.current_branch());
        dump(match.matcher);
    }

    const auto has_catch_all = match.catch_all.has_value();
    {
        const Indent::Guard g{indent_, !has_catch_all};
        fmt::println(out_, "{}Arms:", indent_.current_branch());
        dump_container(match.arms, [this](const MatchExpression::Arm& arm) {
            fmt::println(out_, "{}Arm:", indent_.current_branch());
            {
                const Indent::Guard g_inner{indent_, false};
                fmt::print(out_, "{}Pattern: ", indent_.current_branch());
                dump(arm.pattern);
            }

            if (arm.capture) {
                const Indent::Guard g_inner{indent_, false};
                fmt::print(out_, "{}Capture: ", indent_.current_branch());
                dump(*arm.capture);
            }

            {
                const Indent::Guard g_inner{indent_, true};
                fmt::print(out_, "{}Dispatch: ", indent_.current_branch());
                dump(arm.dispatch);
            }
        });
    }

    if (has_catch_all) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Catch All: ", indent_.current_branch());
        dump(*match.catch_all);
    }
}

#define MAKE_PREFIX_DUMP(NodeType)                                                         \
    auto ASTDumper::visit(NodeID id, const NodeType& node) -> void {                       \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(id.get_token_type())); \
        const Indent::Guard g{indent_, true};                                              \
        fmt::print(out_, "{}Operand: ", indent_.current_branch());                         \
        dump(node.rhs);                                                                    \
    }

MAKE_PREFIX_DUMP(ReferenceExpression)
MAKE_PREFIX_DUMP(AddressOfExpression)
MAKE_PREFIX_DUMP(DereferenceExpression)
MAKE_PREFIX_DUMP(UnaryExpression)

auto ASTDumper::visit(NodeID, const ImplicitAccessExpression& implicit_access) -> void {
    fmt::println(out_, "ImplicitAccessExpression");
    const Indent::Guard g{indent_, true};
    fmt::print(out_, "{}Member: ", indent_.current_branch());
    dump(implicit_access.member);
}

#define MAKE_LEAF_DUMP(NodeType)                                  \
    auto ASTDumper::visit(NodeID, const NodeType& node) -> void { \
        fmt::println(out_, #NodeType ": {}", node);               \
    }

MAKE_LEAF_DUMP(StringExpression)
MAKE_LEAF_DUMP(I32Expression)
MAKE_LEAF_DUMP(I64Expression)
MAKE_LEAF_DUMP(ISizeExpression)
MAKE_LEAF_DUMP(U32Expression)
MAKE_LEAF_DUMP(U64Expression)
MAKE_LEAF_DUMP(USizeExpression)
MAKE_LEAF_DUMP(U8Expression)
MAKE_LEAF_DUMP(F32Expression)
MAKE_LEAF_DUMP(F64Expression)

auto ASTDumper::visit(NodeID id, const BoolExpression&) -> void {
    fmt::println(
        out_, "BoolExpression: {}", id.get_token_type() == syntax::TokenType::BOOLEAN_TRUE);
}

auto ASTDumper::visit(NodeID, const VoidExpression&) -> void {
    fmt::println(out_, "VoidExpression");
}

auto ASTDumper::visit(NodeID, const UndefinedExpression&) -> void {
    fmt::println(out_, "UndefinedExpression");
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const ScopeResolutionExpression& scope_resolve) -> void {
    fmt::println(out_, "ScopeResolutionExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Outer: ", indent_.current_branch());
        dump(scope_resolve.outer);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Inner: ", indent_.current_branch());
        dump(scope_resolve.inner);
    }
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const StructExpression& struct_expr) -> void {
    fmt::println(out_, "StructExpression");
    if (struct_expr.fields.empty() && struct_expr.members.empty()) { return; }

    const auto has_members = !struct_expr.members.empty();
    if (!struct_expr.fields.empty()) {
        const Indent::Guard g{indent_, !has_members};
        fmt::println(out_, "{}Fields:", indent_.current_branch());
        dump_container(struct_expr.fields, [this](const StructExpression::Field& field) {
            {
                fmt::print(out_, "{}Name: ", indent_.current_branch());
                dump(field.name);
            }

            {
                const Indent::Guard g_ident{indent_, false};
                fmt::println(out_, "{}Public: {}", indent_.current_branch(), field.is_public());
            }

            const auto has_default = field.default_value.has_value();
            {
                const Indent::Guard g_type{indent_, !has_default};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump(field.explicit_type);
            }

            if (has_default) {
                const Indent::Guard g_val{indent_, true};
                fmt::print(out_, "{}Default: ", indent_.current_branch());
                dump(*field.default_value);
            }
        });
    }

    if (has_members) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Members:", indent_.current_branch());
        dump_node_list(struct_expr.members);
    }
}

// Safe to call with invalid ID in type dispatch
auto ASTDumper::visit(NodeID, const UnionExpression& union_expr) -> void {
    fmt::println(out_, "UnionExpression");

    const auto has_members = !union_expr.members.empty();
    {
        const Indent::Guard g{indent_, !has_members};
        fmt::println(out_, "{}Fields:", indent_.current_branch());
        dump_container(union_expr.fields, [this](const UnionExpression::Field& field) {
            fmt::println(out_, "{}Field:", indent_.current_branch());
            {
                const Indent::Guard g_pattern{indent_, false};
                fmt::print(out_, "{}Tag: ", indent_.current_branch());
                dump(field.name);
            }

            {
                const Indent::Guard g_result{indent_, true};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump(field.explicit_type);
            }
        });
    }

    if (has_members) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Members:", indent_.current_branch());
        dump_node_list(union_expr.members);
    }
}

auto ASTDumper::visit(NodeID, const WhileLoopExpression& while_expr) -> void {
    fmt::println(out_, "WhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        dump(while_expr.condition);
    }

    if (while_expr.continuation) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Continuation: ", indent_.current_branch());
        dump(*while_expr.continuation);
    }

    const auto has_non_break = while_expr.non_break.has_value();
    {
        const Indent::Guard g{indent_, !has_non_break};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        dump(while_expr.block);
    }

    if (has_non_break) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break Clause: ", indent_.current_branch());
        dump(*while_expr.non_break);
    }
}

auto ASTDumper::visit(NodeID, const Unit&) -> void { fmt::println(out_, "<discarded>"); }

auto ASTDumper::visit(NodeID, const BlockStatement& block) -> void {
    fmt::println(out_, "BlockStatement");
    if (block.empty()) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}<empty>", indent_.current_branch());
    } else {
        dump_node_list(block);
    }
}

auto ASTDumper::visit(NodeID, const BreakStatement& break_stmt) -> void {
    fmt::println(out_, "BreakStatement");
    const auto has_expression = break_stmt.expression.has_value();
    if (break_stmt.label) {
        const Indent ::Guard g{indent_, !has_expression};
        fmt::print(out_, "{}Label: ", indent_.current_branch());
        dump(*break_stmt.label);
    }

    if (has_expression) {
        const Indent ::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        dump(*break_stmt.expression);
    }
}

auto ASTDumper::visit(NodeID, const ContinueStatement& continue_stmt) -> void {
    fmt::println(out_, "ContinueStatement");
    if (continue_stmt.label) {
        const Indent ::Guard g{indent_, true};
        fmt::print(out_, "{}Label: ", indent_.current_branch());
        dump(*continue_stmt.label);
    }
}

auto ASTDumper::visit(NodeID, const DeclStatement& decl) -> void {
    fmt::println(out_, "DeclStatement");

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Name: ", indent_.current_branch());
        dump(decl.name);
    }

    {
        const Indent::Guard g{indent_, false};
        const auto          flags = magic_enum::enum_flags_name(decl.modifiers);
        fmt::println(out_, "{}Modifiers: {}", indent_.current_branch(), flags);
    }

    const auto has_value = decl.value.has_value();
    {
        const Indent::Guard g{indent_, !has_value};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        if (decl.explicit_type) {
            dump(*decl.explicit_type);
        } else {
            fmt::println(out_, "(inferred)");
        }
    }

    if (has_value) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        dump(*decl.value);
    }
}

#define MAKE_BASIC_STMT_DUMP(NodeType, FieldName, field)                      \
    auto ASTDumper::visit(NodeID, const NodeType& node) -> void {             \
        fmt::println(out_, #NodeType);                                        \
        {                                                                     \
            const Indent::Guard g{indent_, true};                             \
            fmt::print(out_, "{}" #FieldName ": ", indent_.current_branch()); \
            dump(node.field);                                                 \
        }                                                                     \
    }

MAKE_BASIC_STMT_DUMP(DeferStatement, Deferred, deferred)
MAKE_BASIC_STMT_DUMP(DiscardStatement, Discarded, discarded)
MAKE_BASIC_STMT_DUMP(ExpressionStatement, Expr, expression)

auto ASTDumper::visit(NodeID id, const ImportStatement& import_stmt) -> void {
    fmt::println(out_, "ImportStatement");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(
            out_, "{}Public: {}", indent_.current_branch(), ImportStatement::is_public(id));
    }

    const auto has_alias = import_stmt.alias.has_value();
    {
        const Indent::Guard g{indent_, !has_alias};
        if (import_stmt.payload.is<IdentifierExpression>()) {
            fmt::print(out_, "{}Library: ", indent_.current_branch());
        } else {
            fmt::print(out_, "{}File: ", indent_.current_branch());
        }
        dump(import_stmt.payload);
    }

    if (has_alias) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        dump(*import_stmt.alias);
    }
}

auto ASTDumper::visit(NodeID, const ReturnStatement& return_stmt) -> void {
    fmt::println(out_, "ReturnStatement");
    if (return_stmt.expression) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        dump(*return_stmt.expression);
    }
}

auto ASTDumper::visit(NodeID, const TestStatement& test) -> void {
    fmt::println(out_, "TestStatement");
    if (test.description) {
        const Indent::Guard g{indent_, false};
        const auto&         string = ast_.get_as<StringExpression>(**test.description);
        fmt::println(out_, "{}Description: {}", indent_.current_branch(), string);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Block: ", indent_.current_branch());
        dump(*test.block);
    }
}

auto ASTDumper::visit(NodeID id, const UsingStatement& using_stmt) -> void {
    fmt::println(out_, "UsingStatement");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Public: {}", indent_.current_branch(), UsingStatement::is_public(id));
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        dump(using_stmt.alias);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        dump(using_stmt.explicit_type);
    }
}

#define MAKE_EXPLICIT_TYPE_DUMP(TypeData)                                 \
    auto ASTDumper::visit(ExplicitTypeID, const TypeData& type) -> void { \
        fmt::print(out_, "{}", indent_.current_branch());                 \
        visit(NodeID::make_invalid(), type);                              \
    }

auto ASTDumper::visit(ExplicitTypeID id, const IdentifierExpression& ident) -> void {
    fmt::print(out_, "{}", indent_.current_branch());
    fmt::print(out_, "IdentifierExpression: {}", ident);
    if (syntax::get_builtin_opt(id.get_token_type())) {
        fmt::print(out_, " (builtin)");
    } else if (syntax::token_type::is_primitive(id.get_token_type())) {
        fmt::print(out_, " (primitive)");
    }
    fmt::println(out_, "");
}

MAKE_EXPLICIT_TYPE_DUMP(ScopeResolutionExpression)
MAKE_EXPLICIT_TYPE_DUMP(DotExpression)
MAKE_EXPLICIT_TYPE_DUMP(CallExpression)

auto ASTDumper::visit(ExplicitTypeID, const ExplicitFunctionType& function) -> void {
    fmt::println(out_, "{}FunctionExpression", indent_.current_branch());
    if (!function.parameter_types.empty()) {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Parameters:", indent_.current_branch());
        dump_container(function.parameter_types, [this](ExplicitTypeID type) {
            fmt::println(out_, "{}Param:", indent_.current_branch());
            {
                const Indent::Guard g_type{indent_, true};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump(type);
            }
        });
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Variadic: {}", indent_.current_branch(), function.variadic);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Returns: ", indent_.current_branch());
        dump(function.explicit_return_type);
    }
}

auto ASTDumper::visit(ExplicitTypeID, const ExplicitTypeID& recursive) -> void {
    fmt::print(out_, "{}", indent_.current_branch());
    dump(recursive);
}

MAKE_EXPLICIT_TYPE_DUMP(StructExpression)
MAKE_EXPLICIT_TYPE_DUMP(EnumExpression)
MAKE_EXPLICIT_TYPE_DUMP(UnionExpression)

auto ASTDumper::visit(ExplicitTypeID, const ExplicitArrayType& array) -> void {
    fmt::println(out_, "{}ArrayType", indent_.current_branch());
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Dimensions: ", indent_.current_branch());
        if (array.dimension) {
            dump(*array.dimension);
        } else {
            fmt::println(out_, "(slice)");
        }
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(
            out_, "{}Null terminated: {}", indent_.current_branch(), array.null_terminated);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}", indent_.current_branch());
        dump(array.inner_explicit_type);
    }
}

} // namespace porpoise::ast
