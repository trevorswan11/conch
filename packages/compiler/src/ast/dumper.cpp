#include <fmt/format.h>
#include <fmt/ostream.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

namespace conch::ast {

#define MAKE_INFIX_DUMP(NodeType, LeftLabel, RightLabel)                             \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        {                                                                            \
            const Indent::Guard g{indent_, false};                                   \
            fmt::print(out_, "{}" #LeftLabel ": ", indent_.current_branch());        \
            node.get_lhs().accept(*this);                                            \
        }                                                                            \
        {                                                                            \
            const Indent::Guard g{indent_, true};                                    \
            fmt::print(out_, "{}" #RightLabel ": ", indent_.current_branch());       \
            node.get_rhs().accept(*this);                                            \
        }                                                                            \
    }

#define MAKE_PREFIX_DUMP(NodeType)                                                   \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        const Indent::Guard g{indent_, true};                                        \
        fmt::print(out_, "{}Operand: ", indent_.current_branch());                   \
        node.get_rhs().accept(*this);                                                \
    }

#define MAKE_LEAF_DUMP(NodeType)                          \
    auto ASTDumper::visit(const NodeType& node) -> void { \
        fmt::println(out_, #NodeType ": {}", node);       \
    }

auto ASTDumper::visit(const ArrayExpression& node) -> void {
    fmt::println(out_, "ArrayExpression");
    {
        const Indent::Guard g{indent_, false};
        if (node.has_explicit_size()) {
            fmt::print(out_, "{}Size: ", indent_.current_branch());
            node.get_explicit_size().accept(*this);
        } else {
            fmt::println(out_, "{}Size: (inferred)", indent_.current_branch());
        }
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        dump_explicit_type(node.get_item_type(), false);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Items: ", indent_.current_branch());
        dump_node_list(node.get_items());
    }
}

auto ASTDumper::visit(const CallExpression& node) -> void {
    fmt::println(out_, "CallExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Callee: ", indent_.current_branch());
        node.get_function().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Arguments: ", indent_.current_branch());
        dump_container(node.get_arguments(), [this](const CallArgument& arg) {
            fmt::print(out_, "{}", indent_.current_branch());
            if (arg.is_expression()) {
                arg.get_expression().accept(*this);
            } else {
                dump_explicit_type(arg.get_type(), false);
            }
        });
    }
}

auto ASTDumper::visit(const DoWhileLoopExpression& node) -> void {
    fmt::println(out_, "DoWhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }
}

auto ASTDumper::visit(const EnumExpression& node) -> void {
    fmt::println(out_, "EnumExpression");

    if (node.has_underlying()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Underlying: ", indent_.current_branch());
        node.get_underlying().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Enumerations:", indent_.current_branch());
        dump_container(node.get_enumerations(), [this](const Enumeration& enumeration) {
            {
                fmt::print(out_, "{}Name: ", indent_.current_branch());
                const Indent::Guard g_name{indent_, !enumeration.has_default_value()};
                enumeration.get_ident().accept(*this);
            }

            if (enumeration.has_default_value()) {
                const Indent::Guard g_val{indent_, true};
                fmt::print(out_, "{}Default: ", indent_.current_branch());
                enumeration.get_default_value().accept(*this);
            }
        });
    }
}

auto ASTDumper::visit(const ForLoopExpression& node) -> void {
    fmt::println(out_, "ForLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Iterables:", indent_.current_branch());
        dump_node_list(node.get_iterables());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Captures:", indent_.current_branch());
        dump_container(node.get_captures(), [this](const auto& capture) {
            if (capture.is_discarded()) {
                fmt::println(out_, "{}<discarded>", indent_.current_branch());
            } else {
                const auto& valued = capture.get_valued();
                fmt::println(out_,
                             "{}{} (modifier: {})",
                             indent_.current_branch(),
                             valued.get_ident(),
                             valued.get_modifier());
            }
        });
    }

    {
        const Indent::Guard g{indent_, !node.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }

    if (node.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break: ", indent_.current_branch());
        node.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const FunctionExpression& node) -> void {
    fmt::println(out_, "FunctionExpression");
    if (node.has_self()) {
        const Indent::Guard g{indent_, false};
        const auto&         self = node.get_self();
        fmt::println(out_,
                     "{}Self: {} (modifier: {})",
                     indent_.current_branch(),
                     self.get_ident(),
                     self.get_modifier());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Parameters:", indent_.current_branch());
        dump_container(node.get_parameters(), [this](const FunctionParameter& param) {
            fmt::println(out_, "{}Param: ", indent_.current_branch());
            {
                const Indent::Guard g_name{indent_, false};
                fmt::print(out_, "{}Name: ", indent_.current_branch());
                param.get_ident().accept(*this);
            }
            {
                const Indent::Guard g_type{indent_, true};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump_explicit_type(param.get_type(), false);
            }
        });
    }

    {
        const Indent::Guard g{indent_, !node.has_body()};
        fmt::print(out_, "{}Returns: ", indent_.current_branch());
        dump_explicit_type(node.get_return_type(), false);
    }

    if (node.has_body()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_body().accept(*this);
    }
}

auto ASTDumper::visit(const IdentifierExpression& node) -> void {
    fmt::print(out_, "IdentifierExpression: {}", node);
    if (node.get_token().is_builtin()) {
        fmt::print(out_, " (builtin)");
    } else if (node.get_token().is_primitive()) {
        fmt::print(out_, " (primitive)");
    }
    fmt::println(out_, "");
}

auto ASTDumper::visit(const IfExpression& node) -> void {
    fmt::println(out_, "IfExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }

    {
        Indent::Guard g{indent_, !node.has_alternate()};
        fmt::print(out_, "{}Then:", indent_.current_branch());
        node.get_consequence().accept(*this);
    }

    if (node.has_alternate()) {
        Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Else:", indent_.current_branch());
        node.get_alternate().accept(*this);
    }
}

auto ASTDumper::visit(const IndexExpression& node) -> void {
    fmt::println(out_, "IndexExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Object: ", indent_.current_branch());
        node.get_array().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Index: ", indent_.current_branch());
        node.get_index().accept(*this);
    }
}

auto ASTDumper::visit(const InfiniteLoopExpression& node) -> void {
    fmt::println(out_, "InfiniteLoopExpression");
    dump_node_list(node.get_block());
}

MAKE_INFIX_DUMP(AssignmentExpression, Assignee, Value)
MAKE_INFIX_DUMP(BinaryExpression, LHS, RHS)
MAKE_INFIX_DUMP(DotExpression, Object, Member)
MAKE_INFIX_DUMP(RangeExpression, Lower, Upper)
MAKE_INFIX_DUMP(ImplicitDereferenceExpression, Object, Member)

auto ASTDumper::visit(const MatchExpression& node) -> void {
    fmt::println(out_, "MatchExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Input: ", indent_.current_branch());
        node.get_matcher().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !node.has_catch_all()};
        fmt::println(out_, "{}Arms:", indent_.current_branch());
        dump_container(node.get_arms(), [this](const MatchArm& arm) {
            fmt::println(out_, "{}Arm", indent_.current_branch());
            {
                const Indent::Guard g_pattern{indent_, false};
                fmt::print(out_, "{}Pattern: ", indent_.current_branch());
                arm.get_pattern().accept(*this);
            }

            if (arm.has_capture_clause()) {
                const Indent::Guard g_pattern{indent_, false};
                fmt::print(out_, "{}Capture: ", indent_.current_branch());
                if (arm.is_explicit_capture()) {
                    arm.get_explicit_capture().accept(*this);
                } else {
                    fmt::println(out_, "<discarded>");
                }
            }

            {
                const Indent::Guard g_result{indent_, true};
                fmt::print(out_, "{}Dispatch: ", indent_.current_branch());
                arm.get_dispatch().accept(*this);
            }
        });
    }

    if (node.has_catch_all()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Catch All: ", indent_.current_branch());
        node.get_catch_all().accept(*this);
    }
}

MAKE_PREFIX_DUMP(ReferenceExpression)
MAKE_PREFIX_DUMP(DereferenceExpression)
MAKE_PREFIX_DUMP(UnaryExpression)
MAKE_PREFIX_DUMP(ImplicitAccessExpression)

MAKE_LEAF_DUMP(StringExpression)
MAKE_LEAF_DUMP(SignedIntegerExpression)
MAKE_LEAF_DUMP(SignedLongIntegerExpression)
MAKE_LEAF_DUMP(ISizeIntegerExpression)
MAKE_LEAF_DUMP(UnsignedIntegerExpression)
MAKE_LEAF_DUMP(UnsignedLongIntegerExpression)
MAKE_LEAF_DUMP(USizeIntegerExpression)
MAKE_LEAF_DUMP(ByteExpression)
MAKE_LEAF_DUMP(FloatExpression)
MAKE_LEAF_DUMP(DoubleExpression)
MAKE_LEAF_DUMP(BoolExpression)

auto ASTDumper::visit(const ScopeResolutionExpression& node) -> void {
    fmt::println(out_, "ScopeResolutionExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Outer: ", indent_.current_branch());
        node.get_outer().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Inner: ", indent_.current_branch());
        node.get_inner().accept(*this);
    }
}

auto ASTDumper::visit(const StructExpression& node) -> void {
    fmt::println(out_, "StructExpression{}", node.is_packed() ? " (packed)" : "");
    dump_node_list(node.get_members());
}

auto ASTDumper::visit(const TypeExpression& node) -> void {
    if (node.has_explicit_type()) {
        dump_explicit_type(node.get_explicit_type(), false);
    } else {
        fmt::println(out_, "(inferred)");
    }
}

auto ASTDumper::visit(const WhileLoopExpression& node) -> void {
    fmt::println(out_, "WhileLoopExpression");

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }

    if (node.has_continuation()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Continuation: ", indent_.current_branch());
        node.get_continuation().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !node.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }

    if (node.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break Clause: ", indent_.current_branch());
        node.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const BlockStatement& node) -> void {
    fmt::println(out_, "BlockStatement");
    if (node.empty()) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}<empty>", indent_.current_branch());
    } else {
        dump_node_list(node);
    }
}

auto ASTDumper::visit(const DeclStatement& node) -> void {
    fmt::println(out_, "DeclStatement ({})", node.get_ident());

    {
        const Indent::Guard g{indent_, false};
        const auto          flags = magic_enum::enum_flags_name(node.get_modifiers());
        fmt::println(out_, "{}Modifiers: {}", indent_.current_branch(), flags);
    }

    {
        const Indent::Guard g{indent_, !node.has_value()};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        node.get_type().accept(*this);
    }

    if (node.has_value()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        node.get_value().accept(*this);
    }
}

auto ASTDumper::visit(const DiscardStatement& node) -> void {
    fmt::println(out_, "DiscardStatement");
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Discarded: ", indent_.current_branch());
        node.get_discarded().accept(*this);
    }
}

auto ASTDumper::visit(const ExpressionStatement& node) -> void {
    fmt::println(out_, "ExpressionStatement");
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Expr: ", indent_.current_branch());
        node.get_expression().accept(*this);
    }
}

auto ASTDumper::visit(const ImportStatement& node) -> void {
    fmt::println(out_, "ImportStatement");
    {
        const Indent::Guard g{indent_, !node.has_alias()};
        if (node.is_module_import()) {
            fmt::print(out_, "{}Module: ", indent_.current_branch());
            node.get_module_import().accept(*this);
        } else {
            fmt::print(out_, "{}User: ", indent_.current_branch());
            node.get_user_import().accept(*this);
        }
    }

    if (node.has_alias()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        node.get_alias().accept(*this);
    }
}

auto ASTDumper::visit(const JumpStatement& node) -> void {
    fmt::println(out_, "JumpStatement ({})", magic_enum::enum_name(node.get_token().type));
    if (node.has_expression()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        node.get_expression().accept(*this);
    }
}

auto ASTDumper::visit(const UsingStatement& node) -> void {
    fmt::println(out_, "UsingStatement");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        node.get_alias().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        dump_explicit_type(node.get_type(), false);
    }
}

auto ASTDumper::dump_explicit_type(const ExplicitType& type, bool print_branch) -> void {
    if (print_branch) { fmt::print(out_, "{}", indent_.current_branch()); }
    fmt::println(out_, "ExplicitType (modifier: {})", type.get_modifier());

    const Indent::Guard g{indent_, true};
    std::visit(
        Overloaded{
            [this](const ExplicitType::ExplicitIdentType& t) {
                fmt::print(out_, "{}", indent_.current_branch());
                t->accept(*this);
            },
            [this](const ExplicitType::ExplicitFunctionType& f) {
                fmt::print(out_, "{}", indent_.current_branch());
                f->accept(*this);
            },
            [this](const ExplicitArrayType& a) {
                fmt::println(out_, "{}ArrayType", indent_.current_branch());
                {
                    const Indent::Guard g_inner{indent_, false};
                    fmt::print(out_, "{}Dimensions: ", indent_.current_branch());
                    if (a.has_dimension()) {
                        a.get_dimension().accept(*this);
                    } else {
                        fmt::println(out_, "(slice)");
                    }
                }
                {
                    const Indent::Guard g_inner{indent_, true};
                    dump_explicit_type(a.get_inner_type(), true);
                }
            },
            [this](const ExplicitType::ExplicitRecursiveType& r) { dump_explicit_type(*r, true); },
        },
        type.get_type());
}

} // namespace conch::ast
