#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "ast/handle.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "module/memory_loader.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"

#include "option.hh"
#include "sema/type.hh"
#include "syntax/error.hh"
#include "types.hh"

namespace ghoti::tests::helpers {

auto test_common_decl_collection(const sema::SymbolTableRegistry& registry,
                                 const mod::Module&               module,
                                 usize                            idx,
                                 std::string_view                 name) -> void {
    const auto& symbol = unwrap(registry.get_from_opt(idx, name));
    const auto& node   = unwrap(symbol.get_data().as_opt<sema::symbols::Node>());
    CHECK_FALSE(symbol.is_public(module));
    CHECK(node.is<ast::DeclStatement>());
}

SemaTestContext::SemaTestContext(const std::vector<MockFile>& imports,
                                 const std::filesystem::path& root_path,
                                 std::string_view             input,
                                 std::ostream&                error_stream)
    : loader{mem::make_box<mod::MemoryLoader>()}, manager{*loader},
      analyzer{manager, error_stream, false}, root_mod{[&] {
          loader->add(root_path, std::string{input});
          for (const auto& mock : imports) {
              loader->add(mock.path, std::string{mock.source});
              if (mock.name) { REQUIRE(manager.add_library_module(*mock.name, mock.path)); }
          }

          return unwrap(manager.try_get_file_module(root_path));
      }()} {}

auto SemaTestContext::verify_registry_resolved() -> void {
    for (usize i = 0; const auto& table : analyzer.get_registry()) {
        for (const auto& [name, proxy] : table) {
            const auto& symbol = proxy.symbol;
            CHECK(symbol.get_status() == sema::SymbolStatus::RESOLVED);
            if (symbol.get_status() != sema::SymbolStatus::RESOLVED) {
                FAIL(name << " was not resolved in table idx " << i);
            }
        }
        i++;
    }
}

auto SemaTestContext::test_common_decl_collection(usize idx, std::string_view name) -> void {
    const auto& registry = analyzer.get_registry();
    helpers::test_common_decl_collection(registry, *root_mod, idx, name);
}

auto SemaTestContext::check_poisoned(const sema::Symbol& sym) -> void {
    CHECK(sym.get_kind_opt() == sema::SymbolKind::POISONED);
    CHECK(sym.get_status() == sema::SymbolStatus::RESOLVED);
}

auto SemaTestContext::check_poisoned(const sema::Type& type) -> void {
    CHECK(type.is_poison());
    CHECK(type == get_type(sema::TypeKind::POISON));
    CHECK(type.get_data().as_opt<sema::types::Poison>());
}

auto SemaTestContext::check_poisoned(const sema::Symbol& sym, const sema::Type& type) -> void {
    check_poisoned(sym);
    check_poisoned(type);
}

auto SemaTestContext::get_string_literal_size(ast::ExpressionHandle     handle,
                                              opt::Option<mod::Module&> enclosing_mod) -> usize {
    const auto& module   = enclosing_mod.value_or(*root_mod);
    const auto& str_expr = helpers::unwrap(module.ast.get_as_opt<ast::StringExpression>(handle));
    return str_expr.value.size() + 1;
}

auto collect(std::string_view input, const std::vector<MockFile>& imports) -> CtxIdxPair {
    auto ctx = mem::make_box<SemaTestContext>(imports, TEST_FILENAME, input);
    check_errors<syntax::Diagnostics>(*ctx->root_mod);
    ctx->analyzer.collect_symbols(*ctx->root_mod);

    REQUIRE(ctx->root_mod->root_table_idx);
    usize idx = *ctx->root_mod->root_table_idx;
    return {std::move(ctx), idx};
}

auto collect_and_check(std::string_view input, const std::vector<MockFile>& imports) -> CtxIdxPair {
    auto [ctx, idx] = collect(input, imports);
    check_errors<sema::Diagnostics>(*ctx->root_mod);
    return {std::move(ctx), idx};
}

auto resolve(std::string_view input, const std::vector<MockFile>& imports) -> CtxIdxPair {
    auto [ctx, idx] = collect(input, imports);
    ctx->analyzer.resolve_types(*ctx->root_mod);
    return {std::move(ctx), idx};
}

auto resolve_and_check(std::string_view input, const std::vector<MockFile>& imports) -> CtxIdxPair {
    auto [ctx, idx] = resolve(input, imports);
    check_errors<sema::Diagnostics>(*ctx->root_mod);
    ctx->verify_registry_resolved();

    return {std::move(ctx), idx};
}

} // namespace ghoti::tests::helpers
