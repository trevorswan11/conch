#include "sema/passes/symbol_collector.hh"

#include <string_view>
#include <utility>
#include <variant>

#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/visitor.hh"
#include "module/error.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "assert.hh"
#include "iterator.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::sema {

auto SymbolCollector::collect_symbols(mod::Module& module, Context& ctx) -> mod::ModuleState {
    if (module.is_collectable()) {
        module.root_table_idx.emplace(ctx.registry.create());

        SymbolCollector collector{module, ctx};
        for (const auto& node : module.ast) { collector.collect(node); }

        if (!ctx.diagnostics.empty()) {
            return module.error_out(std::move(ctx.diagnostics),
                                    mod::ModuleState::POISONED_SYMBOL_COLLECTION);
        }
        module.state = mod::ModuleState::SYMBOLS_COLLECTED;
    }
    return module.state;
}

// Many terminal expressions are skipped on this pass
#define MAKE_COLLECTOR_NOOPS(X) \
    X(IdentifierExpression)     \
    X(DotExpression)            \
    X(ImplicitAccessExpression) \
    X(StringExpression)         \
    X(I32Expression)            \
    X(I64Expression)            \
    X(ISizeExpression)          \
    X(U32Expression)            \
    X(U64Expression)            \
    X(USizeExpression)          \
    X(U8Expression)             \
    X(F32Expression)            \
    X(F64Expression)            \
    X(BoolExpression)           \
    X(VoidExpression)           \
    X(UndefinedExpression)      \
    X(ScopeResolutionExpression)

#define COLLECTOR_NOOP_X(NodeType) AST_NODE_VISITOR_NOOP(SymbolCollector, NodeType)
MAKE_COLLECTOR_NOOPS(COLLECTOR_NOOP_X)
#undef COLLECTOR_NOOP_X

auto SymbolCollector::visit(ast::NodeID, const ast::ArrayExpression& array) -> void {
    const auto g = in_expr_scope_.guard();
    if (array.size) { collect(*array.size); }
    collect(array.item_explicit_type);
    for (const auto& item : array.items) { collect(*item); }
}

auto SymbolCollector::visit(ast::NodeID, const ast::CallExpression& call) -> void {
    const auto g = in_expr_scope_.guard();
    collect(call.function);
    for (const auto& arg : call.arguments) {
        std::visit([this](auto arg_id) { collect(arg_id); }, arg.id);
    }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::DoWhileLoopExpression& do_while) -> void {
    const auto  g     = loop_guard();
    const auto& block = collecting_.ast.get_as<ast::BlockStatement>(do_while.block);
    const auto  idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(idx);
    collecting_.set_sema_type(id, *last_type_.take());

    // The condition is just an expression
    {
        const auto g_cond = in_expr_scope_.guard();
        collect(do_while.condition);
    }
}

template <traits::IndexableID ID>
auto SymbolCollector::visit(ID id, const ast::EnumExpression& enum_expr) -> void {
    const auto scope_idx = visit_scopes(
        TypeKind::ENUM,
        IterPair{enum_expr.enumerations,
                 [this](const ast::EnumExpression::Enumeration& enumeration) {
                     if (enumeration.value) { collect(*enumeration.value); }

                     // Resolve the ident second to prevent self-referential values
                     const auto& ident =
                         collecting_.ast.get_as<ast::IdentifierExpression>(enumeration.name);
                     try_declare<symbols::Enumeration>(ident.name, enumeration);
                 }},
        IterPair{enum_expr.members, [this](const ast::MemberHandle& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.set_sema_type(id, *last_type_);
}

VISITOR_TEMPLATE_INIT(SymbolCollector, visit, EnumExpression)

auto SymbolCollector::visit(ast::NodeID id, const ast::ForLoopExpression& for_expr) -> void {
    // The guard shouldn't enclose the else clause
    const auto g_expr = in_expr_scope_.guard();
    for (const auto& iterable : for_expr.iterables) { collect(iterable); }

    usize new_idx;
    {
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto g_loop = in_loop_scope_.guard();
        for (const auto& capture : for_expr.captures) {
            if (const auto ident =
                    collecting_.ast.get_as_opt<ast::IdentifierExpression>(capture.payload)) {
                try_declare<symbols::ForLoopCapture>(ident->name, capture);
            }
        }
        const auto& block = collecting_.ast.get_as<ast::BlockStatement>(for_expr.block);
        for (const auto& stmt : block) { collect(stmt); }
    }
    if (for_expr.non_break) { collect(*for_expr.non_break); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, types::mut::CONSTANT, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(ast::NodeID id, const ast::FunctionExpression& fn) -> void {
    const auto  new_idx = ctx_.registry.create();
    const Scope s{table_stack_, new_idx, table_idx_};
    const auto  g = fn_guard();

    // Parameters belong to the parent scope
    if (fn.self) {
        const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(fn.self->name);
        try_declare<symbols::SelfParameter>(ident.name, fn.self.get());
    }

    // The parameter's type should be collected first to prevent self-referential types
    for (const auto& param : fn.parameters) {
        collect(param.explicit_type);
        const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(param.name);
        try_declare<symbols::Parameter>(ident.name, param);
    }
    collect(fn.explicit_return_type);

    const auto& block = collecting_.ast.get_as<ast::BlockStatement>(fn.body);
    for (const auto& stmt : block) { collect(stmt); }

    last_type_.emplace(ctx_.pool[{TypeKind::FUNCTION, types::mut::CONSTANT, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.set_sema_type(id, *last_type_);
}

auto SymbolCollector::visit(ast::NodeID, const ast::IfExpression& if_expr) -> void {
    const auto g = in_expr_scope_.guard();
    collect(if_expr.condition);
    collect(if_expr.consequence);
    if (if_expr.alternate) { collect(*if_expr.alternate); }
}

auto SymbolCollector::visit(ast::NodeID, const ast::IndexExpression& index) -> void {
    const auto g = in_expr_scope_.guard();
    collect(index.index);
}

auto SymbolCollector::visit(ast::NodeID id, const ast::InfiniteLoopExpression& loop) -> void {
    const auto  g     = loop_guard();
    const auto& block = collecting_.ast.get_as<ast::BlockStatement>(loop.block);
    const auto  idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

#define MAKE_INFIX_COLLECTOR(NodeType)                                            \
    auto SymbolCollector::visit(ast::NodeID, const ast::NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                                    \
        collect(node.lhs);                                                        \
        collect(node.rhs);                                                        \
    }

MAKE_INFIX_COLLECTOR(RangeExpression)
MAKE_INFIX_COLLECTOR(AssignmentExpression)
MAKE_INFIX_COLLECTOR(BinaryExpression)

auto SymbolCollector::visit(ast::NodeID, const ast::InitializerExpression& init) -> void {
    const auto g = in_expr_scope_.guard();
    for (const auto& initializer : init.initializers) { collect(initializer.value); }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::LabelExpression& label) -> void {
    const auto  g     = label_guard();
    const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(label.name);

    // Labels and their associated nodes live in their own scope
    const auto  new_idx = ctx_.registry.create();
    const Scope s{table_stack_, new_idx, table_idx_};
    if (try_declare<symbols::Label>(ident.name, symbols::Label::Handle{id})) {
        auto& symbol = ctx_.registry.get_from(table_idx_, ident.name);
        symbol.set_kind(SymbolKind::LABEL);
    }
    collect(label.body);

    last_type_.emplace(ctx_.pool[{TypeKind::LABEL, types::mut::CONSTANT, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(ast::NodeID, const ast::MatchExpression& match) -> void {
    const auto g = in_expr_scope_.guard();
    collect(match.matcher);
    for (const auto& arm : match.arms) {
        const auto  new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        collect(arm.pattern);
        if (arm.capture && (*arm.capture)->get_kind() == ast::NodeKind::IDENTIFIER_EXPRESSION) {
            const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(*arm.capture);
            try_declare<symbols::MatchCapture>(ident.name, *arm.capture);
        }
        collect(arm.dispatch);

        last_type_.emplace(ctx_.pool[{TypeKind::MATCH_ARM, types::mut::CONSTANT, new_idx}]);
        last_type_->set_symbol_table_idx(new_idx);
        collecting_.set_sema_type(arm, *last_type_.take());
    }
    if (match.catch_all) { collect(*match.catch_all); }
}

#define MAKE_PREFIX_COLLECTOR(NodeType)                                           \
    auto SymbolCollector::visit(ast::NodeID, const ast::NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                                    \
        collect(node.rhs);                                                        \
    }

MAKE_PREFIX_COLLECTOR(ReferenceExpression)
MAKE_PREFIX_COLLECTOR(AddressOfExpression)
MAKE_PREFIX_COLLECTOR(DereferenceExpression)
MAKE_PREFIX_COLLECTOR(UnaryExpression)

template <traits::IndexableID ID>
auto SymbolCollector::visit(ID id, const ast::StructExpression& struct_expr) -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::STRUCT,
                     IterPair{struct_expr.fields,
                              [this](const ast::StructExpression::Field& field) {
                                  if (field.default_value) { collect(*field.default_value); }
                                  collect(field.explicit_type);

                                  // Resolve the ident last to prevent self-referential values
                                  const auto& ident =
                                      collecting_.ast.get_as<ast::IdentifierExpression>(field.name);
                                  try_declare<symbols::StructField>(ident.name, field);
                              }},
                     IterPair{struct_expr.members,
                              [this](const ast::MemberHandle& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.set_sema_type(id, *last_type_);
}

VISITOR_TEMPLATE_INIT(SymbolCollector, visit, StructExpression)

template <traits::IndexableID ID>
auto SymbolCollector::visit(ID id, const ast::UnionExpression& union_expr) -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::UNION,
                     IterPair{union_expr.fields,
                              [this](const ast::UnionExpression::Field& field) {
                                  // Resolve the ident second to prevent self-referential values
                                  collect(field.explicit_type);

                                  const auto& ident =
                                      collecting_.ast.get_as<ast::IdentifierExpression>(field.name);
                                  try_declare<symbols::UnionField>(ident.name, field);
                              }},
                     IterPair{union_expr.members,
                              [this](const ast::MemberHandle& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.set_sema_type(id, *last_type_);
}

VISITOR_TEMPLATE_INIT(SymbolCollector, visit, UnionExpression)

auto SymbolCollector::visit(ast::NodeID id, const ast::WhileLoopExpression& while_expr) -> void {
    // The guard shouldn't enclose the else clause or condition
    const auto g_expr = in_expr_scope_.guard();
    collect(while_expr.condition);
    if (while_expr.continuation) { collect(*while_expr.continuation); }

    usize new_idx;
    {
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto  g_loop = in_loop_scope_.guard();
        const auto& block  = collecting_.ast.get_as<ast::BlockStatement>(while_expr.block);
        for (const auto& stmt : block) { collect(stmt); }
    }
    if (while_expr.non_break) { collect(*while_expr.non_break); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, types::mut::CONSTANT, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(ast::NodeID id, const ast::BlockStatement& block) -> void {
    if (!in_expr_scope_ && table_stack_.size() == 1) {
        ctx_.diagnostics.emplace_back("Cannot have block at the top level",
                                      Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                      collecting_.ast.location_of(id));
    }

    const auto scope_idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(ast::NodeID id, const ast::BreakStatement& break_stmt) -> void {
    if (!in_loop_scope_ && !in_label_scope_) {
        ctx_.diagnostics.emplace_back("Cannot break outside of a loop or label",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.ast.location_of(id));
    }
    if (break_stmt.expression) { collect(*break_stmt.expression); }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::ContinueStatement&) -> void {
    if (!in_loop_scope_) {
        ctx_.diagnostics.emplace_back("Cannot continue outside of a loop",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.ast.location_of(id));
    }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::DeclStatement& decl) -> void {
    // We can stop analyzing early if there's no value
    const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(decl.name);
    const auto  name  = ident.name;
    if (!try_declare<symbols::Node>(name, id)) { return; };
    if (!decl.value) { return; }

    // Valued decls should be evaluated to get shallow types
    auto&      symbol = ctx_.registry.get_from(table_idx_, name);
    const auto value  = *decl.value;
    if (value.any<ast::EnumExpression, ast::UnionExpression, ast::StructExpression>()) {
        if (decl.has_modifier(ast::DeclModifiers::CONSTEXPR)) {
            ctx_.diagnostics.emplace_back(
                fmt::format("All {}s are implicitly constexpr", value->display_name()),
                Error::REDUNDANT_CONSTEXPR,
                collecting_.ast.location_of(id));
        } else if (!decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            ctx_.poison_symbol(symbol,
                               fmt::format("All {}s must be marked const", value->display_name()),
                               Error::ILLEGAL_NON_CONST_STATEMENT,
                               collecting_.ast.location_of(id));
        } else {
            symbol.set_kind(SymbolKind::TYPE);
        }
    } else if (value.is<ast::FunctionExpression>()) {
        if (!decl.has_modifier(ast::DeclModifiers::CONSTEXPR) &&
            !decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            ctx_.poison_symbol(symbol,
                               "All function declarations must be const or constexpr",
                               Error::ILLEGAL_NON_CONST_STATEMENT,
                               collecting_.ast.location_of(id));
        } else {
            symbol.set_kind(SymbolKind::CALLABLE);
        }
    }

    collect(value);
    if (last_type_) { collecting_.set_sema_type(value, *last_type_.take()); }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::DeferStatement& defer) -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot have defer outside of a function's scope",
                                      Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                      collecting_.ast.location_of(id));
    }
    collect(defer.deferred);
}

auto SymbolCollector::visit(ast::NodeID, const ast::DiscardStatement& discard) -> void {
    collect(discard.discarded);
}

auto SymbolCollector::visit(ast::NodeID, const ast::ExpressionStatement& expr) -> void {
    collect(expr.expression);
}

auto SymbolCollector::collect_import_payload(const ast::ImportStatement& import_stmt)
    -> std::pair<std::string_view, Result<mem::NonNull<mod::Module>, mod::Diagnostic>> {
    const auto name = import_stmt.get_name(collecting_.ast);
    if (const auto string =
            collecting_.ast.get_as_opt<ast::StringExpression>(import_stmt.payload)) {
        ASSERT(import_stmt.alias, "File import without alias");
        return {name, ctx_.modules.try_get_file_module(string->value, collecting_.parent_path)};
    }

    const auto& payload = collecting_.ast.get_as<ast::IdentifierExpression>(import_stmt.payload);
    return {name, ctx_.modules.try_get_library_module(payload.name)};
}

auto SymbolCollector::visit(ast::NodeID id, const ast::ImportStatement& import_stmt) -> void {
    auto [alias, mod_result] = collect_import_payload(import_stmt);

    // Only set the table index if the module exists
    opt::Option<mod::Module&> imported_mod;
    if (!mod_result) {
        // The token is retrieved here to avoid copying it on success
        ctx_.diagnostics.emplace_back(mod_result.error().get_message(),
                                      Error::MODULE_LOAD_ERROR,
                                      collecting_.ast.location_of(*import_stmt.payload));
    } else {
        imported_mod.emplace(**mod_result);
        Context new_ctx = ctx_;
        collect_symbols(*imported_mod, new_ctx);
    }

    try_declare<symbols::Node>(alias, id);
    if (imported_mod && !imported_mod->is_errored()) {
        // Its much easier for other steps to get the enclosing module if we resolve now
        auto& type =
            ctx_.pool[{TypeKind::MODULE, types::mut::CONSTANT, *imported_mod->root_table_idx}];
        type.resolve<types::Module>(*imported_mod);

        collecting_.set_sema_type(id, type);
        auto& symbol = ctx_.registry.get_from(table_idx_, alias);
        symbol.set_status(SymbolStatus::RESOLVED);
        symbol.set_kind(SymbolKind::MODULE);
    } else {
        ctx_.poison_symbol(ctx_.registry.get_from(table_idx_, alias));
        ctx_.poison_node(collecting_, id);
    }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::ReturnStatement& return_stmt) -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot return outside of a function",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.ast.location_of(id));
    }
    if (return_stmt.expression) { collect(*return_stmt.expression); }
}

auto SymbolCollector::visit(ast::NodeID id, const ast::TestStatement& test) -> void {
    if (table_stack_.size() != 1) {
        ctx_.diagnostics.emplace_back("Tests must be at the topmost level of a file",
                                      Error::ILLEGAL_TEST_LOCATION,
                                      collecting_.ast.location_of(id));
    }

    // Not a symbol so don't push to the table, track in node instead
    const auto& block = collecting_.ast.get_as<ast::BlockStatement>(test.block);
    const auto  scope_idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(ast::NodeID id, const ast::UsingStatement& using_stmt) -> void {
    collect(using_stmt.explicit_type);
    const auto& ident = collecting_.ast.get_as<ast::IdentifierExpression>(using_stmt.alias);
    try_declare<symbols::Node>(ident.name, id);
    ctx_.registry.get_from(table_idx_, ident.name).set_kind(SymbolKind::TYPE);
}

AST_TYPE_VISITOR_NOOP(SymbolCollector, IdentifierExpression)
AST_TYPE_VISITOR_NOOP(SymbolCollector, ScopeResolutionExpression)
AST_TYPE_VISITOR_NOOP(SymbolCollector, DotExpression)

auto SymbolCollector::visit(ast::ExplicitTypeID, const ast::CallExpression& call) -> void {
    visit(ast::NodeID::make_invalid(), call);
}

auto SymbolCollector::visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType& fn) -> void {
    for (const auto& param : fn.parameter_types) { collect(param); }
    collect(fn.explicit_return_type);
}

auto SymbolCollector::visit(ast::ExplicitTypeID, const ast::ExplicitArrayType& array) -> void {
    const auto g = in_expr_scope_.guard();
    if (array.dimension) { collect(*array.dimension); }
    collect(array.inner_explicit_type);
}

} // namespace porpoise::sema
