#include "sema/symbol.hh"

#include <ranges>
#include <string_view>

#include <fmt/format.h>

#include "ast/kind.hh"
#include "ast/statement.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace ghoti::sema {

auto symbols::Label::from(Symbol& symbol) -> Label& {
    auto label_data = symbol.as_opt<symbols::Label>();
    ASSERT(label_data, "Label is not linked to a symbolic label");
    return *label_data;
}

namespace {

[[nodiscard]] auto symbol_location_of(const mod::Module& module, const Symbol::Data& data) noexcept
    -> SourceLocation {
    return data.visit(Overloaded{
        [](const symbols::Builtin&) { return SourceLocation{0, 0}; },
        [&module](const auto& handle) { return module.ast.location_of(handle); },
        [&module](const symbols::Label& label) {
            return module.ast.location_of(label.get_definition());
        },
        [&module](const symbols::StructField& inner) { return module.ast.location_of(inner.name); },
        [&module](const symbols::UnionField& inner) { return module.ast.location_of(inner.name); },
        [&module](const symbols::Enumeration& inner) { return module.ast.location_of(inner.name); },
        [&module](const symbols::SelfParameter& inner) {
            return module.ast.location_of(inner.name);
        },
        [&module](const symbols::Parameter& inner) { return module.ast.location_of(inner.name); },
        [&module](const symbols::ForLoopCapture& inner) {
            return module.ast.location_of(inner.payload);
        }});
}

} // namespace

auto Symbol::get_symbol_location(const mod::Module& module) const noexcept -> SourceLocation {
    return symbol_location_of(module, data_);
}

auto Symbol::is_public(const mod::Module& module) const noexcept -> bool {
    return data_.visit(
        Overloaded{[&module](const symbols::Node& node) {
                       switch (node->get_kind()) {
                       case ast::NodeKind::DECL_STATEMENT:
                           return module.ast.get_as<ast::DeclStatement>(*node).has_modifier(
                               ast::DeclModifiers::PUBLIC);
                       case ast::NodeKind::USING_STATEMENT:
                       case ast::NodeKind::IMPORT_STATEMENT:
                           return node->get_token_type() == syntax::TokenType::PUBLIC;
                       default: return false;
                       }
                   },
                   [](const auto&) { return false; }});
}

auto SymbolTable::insert(std::string_view name, const mod::Module& module, const Symbol::Data& data)
    -> Result<void, Diagnostic> {
    // Reserved identifier use is impossible due to a parser invariant
    auto [it, inserted] = symbols_.try_emplace(name, Symbol{name, data}, symbols_.size());

    // Check for redeclaration since there's no shadowing
    if (!inserted) {
        return make_sema_err(
            fmt::format("Redeclaration of symbol '{}'; previous declaration here: {}",
                        name,
                        it->second.symbol.get_symbol_location(module)),
            Error::IDENTIFIER_REDECLARATION,
            symbol_location_of(module, data));
    }
    return {};
}

auto SymbolTable::insert_unchecked(std::string_view name, const Symbol::Data& data) -> void {
    // Reserved identifier use is impossible due to a parser invariant
    auto [_, inserted] = symbols_.try_emplace(name, Symbol{name, data}, symbols_.size());
    ASSERT(inserted, "Duplicate symbol injected");
}

auto SymbolTableRegistry::insert_into(usize               table_idx,
                                      const mod::Module&  module,
                                      std::string_view    name,
                                      const Symbol::Data& data) -> Result<void, Diagnostic> {
    if (auto table = get_opt(table_idx)) { return table->insert(name, module, data); }
    return make_sema_err(Error::INVALID_TABLE_IDX);
}

[[nodiscard]] auto SymbolTableRegistry::is_shadowing(const SymbolTableStack& stack,
                                                     const mod::Module&      module,
                                                     std::string_view        name,
                                                     const Symbol::Data&     data) noexcept
    -> Result<void, Diagnostic> {
    for (const auto idx : stack | std::views::take(stack.size() - 1)) {
        if (const auto symbol = get(idx).get_opt(name)) {
            return make_sema_err(
                fmt::format("Attempt to shadow identifier '{}'; previous declaration here: {}",
                            name,
                            symbol_location_of(module, symbol->get_data())),
                Error::SHADOWING_DECLARATION,
                symbol_location_of(module, data));
        }
    }
    return {};
}

} // namespace ghoti::sema
