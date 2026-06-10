#pragma once

#include <concepts>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ostream>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gsl/pointers>

#include "ast/handle.hh"
#include "ast/kind.hh"
#include "helpers/common.hh"
#include "module/memory_loader.hh"
#include "module/module.hh"
#include "sema/analyzer.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/error.hh"

#include <memory.hh>
#include <option.hh>
#include <types.hh>
#include <utility.hh>
#include <variant.hh>

namespace ghoti::tests::helpers {

constexpr std::string_view TEST_FILENAME{"test.gh"};

struct MockFile {
    std::string_view              path;
    std::string_view              source;
    opt::Option<std::string_view> name{}; // NOLINT
};

// Tests the collected state of a minimal non-public implicit declaration
auto test_common_decl_collection(const sema::SymbolTableRegistry& registry,
                                 const mod::Module&               module,
                                 usize                            idx,
                                 std::string_view                 name = "foo") -> void;

template <typename Data>
concept ASTData = traits::ASTNode<Data> || traits::ASTExplicitType<Data>;

struct SemaTestContext {
    mem::Box<mod::MemoryLoader> loader;
    mod::ModuleManager          manager;
    sema::Analyzer              analyzer;
    mod::Module&                root_mod;

    // The root is automatically added to the internal loader and can be immediately analyzed
    explicit SemaTestContext(const std::vector<MockFile>& imports,
                             const std::filesystem::path& root_path,
                             std::string_view             input,
                             std::ostream&                error_stream = std::cerr);
    ~SemaTestContext() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SemaTestContext)

    // Walks all tables in the registry and checks that all symbols are resolved
    //
    // A successful resolution should resolve every single symbol
    auto verify_registry_resolved() -> void;

    // Tests the collected state of a minimal non-public implicit declaration in the context
    auto test_common_decl_collection(usize idx, std::string_view name = "foo") -> void;

    template <sema::types::MutabilityModifiers Mutability = sema::types::mut::CONSTANT,
              typename... Markers>
    [[nodiscard]] auto get_type(sema::TypeKind kind, Markers&&... markers) -> auto& {
        return *analyzer.get_pool()[{kind, Mutability, std::forward<Markers>(markers)...}];
    }

    static auto check_poisoned(const sema::Symbol& sym) -> void;
    auto        check_poisoned(const sema::Type& type) -> void;
    auto        check_poisoned(const sema::Symbol& sym, const sema::Type& type) -> void;

    template <typename SymbolData, typename Proj = std::identity>
    auto check_poisoned(std::string_view          name,
                        usize                     table_idx,
                        opt::Option<mod::Module&> module = opt::none,
                        Proj                      proj   = {}) -> void {
        const auto [sym, _, type] = get_type_sym_info<SymbolData>(name, table_idx, module, proj);
        check_poisoned(sym, type);
    }

    // Contains [symbol, symbol data]
    template <typename SymbolData>
    [[nodiscard]] auto get_symbol(std::string_view name, usize table_idx) {
        const auto& symbol = helpers::unwrap(analyzer.get_registry().get_from_opt(table_idx, name));
        const auto& symbol_data = helpers::unwrap(symbol.get_data().as_opt<SymbolData>());
        return std::tie(symbol, symbol_data);
    }

    // Contains [symbol, symbol data, type]
    template <typename SymbolData, typename Proj = std::identity>
    [[nodiscard]] auto get_type_sym_info(std::string_view          name,
                                         usize                     table_idx,
                                         opt::Option<mod::Module&> module = opt::none,
                                         Proj                      proj   = {}) {
        auto        info      = get_symbol<SymbolData>(name, table_idx);
        auto&       enclosing = module.value_or(root_mod);
        const auto& type =
            helpers::unwrap(enclosing.get_sema_type_opt(std::invoke(proj, std::get<1>(info))));
        return std::tuple_cat(info, std::forward_as_tuple(type));
    }

    // Contains [symbol, symbol data, type, type data]
    template <typename SymbolData, typename TypeData, typename Proj = std::identity>
    [[nodiscard]] auto get_full_type_sym_info(std::string_view          name,
                                              usize                     table_idx,
                                              opt::Option<mod::Module&> module = opt::none,
                                              Proj                      proj   = {}) {
        auto        info = get_type_sym_info<SymbolData>(name, table_idx, module, proj);
        const auto& type_data =
            helpers::unwrap(std::get<2>(info).get_data().template as_opt<TypeData>());
        return std::tuple_cat(info, std::forward_as_tuple(type_data));
    }

    // Contains [symbol, symbol data, ast data]
    template <typename SymbolData, ASTData TreeData, typename Proj = std::identity>
    [[nodiscard]] auto get_ast_sym_info(std::string_view          name,
                                        usize                     table_idx,
                                        opt::Option<mod::Module&> module = opt::none,
                                        Proj                      proj   = {}) {
        auto        info      = get_symbol<SymbolData>(name, table_idx);
        auto&       enclosing = module.value_or(root_mod);
        const auto& node_data = helpers::unwrap(
            enclosing.ast.get_as_opt<TreeData>(std::invoke(proj, std::get<1>(info))));
        return std::tuple_cat(info, std::forward_as_tuple(node_data));
    }

    // Contains the [symbol, symbol data, ast data, type]
    template <typename SymbolData, ASTData TreeData, typename Proj = std::identity>
    [[nodiscard]] auto get_ast_type_sym_info(std::string_view          name,
                                             usize                     table_idx,
                                             opt::Option<mod::Module&> module = opt::none,
                                             Proj                      proj   = {}) {
        auto        info = get_ast_sym_info<SymbolData, TreeData>(name, table_idx, module, proj);
        auto&       enclosing = module.value_or(root_mod);
        const auto& type =
            helpers::unwrap(enclosing.get_sema_type_opt(std::invoke(proj, std::get<1>(info))));
        return std::tuple_cat(info, std::forward_as_tuple(type));
    }

    // Contains [symbol, symbol data, ast data, type, type data]
    template <typename SymbolData, ASTData TreeData, typename TypeData>
    [[nodiscard]] auto get_full_sym_info(std::string_view          name,
                                         usize                     table_idx,
                                         opt::Option<mod::Module&> module = opt::none) {
        auto        info = get_ast_type_sym_info<SymbolData, TreeData>(name, table_idx, module);
        const auto& type_data =
            helpers::unwrap(std::get<3>(info).get_data().template as_opt<TypeData>());
        return std::tuple_cat(info, std::forward_as_tuple(type_data));
    }

    // Returns the correct null terminated size for a string literal, defaulting to the root module
    auto get_string_literal_size(ast::ExpressionHandle     handle,
                                 opt::Option<mod::Module&> enclosing_mod = opt::none) -> usize;
};

using CtxIdxPair = std::pair<mem::Box<SemaTestContext>, usize>;

// Collects the assumed-syntactically-valid input and returns the analyzer and parent table index
[[nodiscard]] auto collect(std::string_view input, const std::vector<MockFile>& imports = {})
    -> CtxIdxPair;

// Collects the assumed-syntactically-valid input and checks for no errors
auto collect_and_check(std::string_view input, const std::vector<MockFile>& imports = {})
    -> CtxIdxPair;

// Resolves the input and returns the parent table index
[[nodiscard]] auto resolve(std::string_view input, const std::vector<MockFile>& imports = {})
    -> CtxIdxPair;

// Resolves the input, checks errors, asserts 100% symbol resolution, and returns the parent index
auto resolve_and_check(std::string_view input, const std::vector<MockFile>& imports = {})
    -> CtxIdxPair;

// Runs the entire Analyzer on the provided input without checking semantic validity (no errors)
template <std::same_as<MockFile>... Mocks>
auto analyze(std::string_view root_path,
             std::ostream&    error_stream,
             std::string_view input,
             Mocks&&... mocks) -> mem::Box<SemaTestContext> {
    auto ctx = mem::make_box<SemaTestContext>(
        make_vector<MockFile>(std::forward<Mocks>(mocks)...), root_path, input, error_stream);
    check_errors<syntax::Diagnostics>(ctx->root_mod);

    auto& analyzer = ctx->analyzer;
    REQUIRE(analyzer.analyze(root_path));
    return ctx;
}

// Runs the entire Analyzer on the provided input and checks for errors
template <std::same_as<MockFile>... Mocks>
auto analyze_and_check(std::string_view root_path, std::string_view input, Mocks&&... mocks)
    -> mem::Box<SemaTestContext> {
    auto ctx = analyze(root_path, std::cerr, input, std::forward<Mocks>(mocks)...);
    REQUIRE_FALSE(ctx->root_mod.diagnostics.template is<Unit>());
    return ctx;
}

// Tests a semantically failing input against the expected generated errors
template <std::same_as<sema::Diagnostic>... Ds>
auto test_collector_fail(std::string_view             failing,
                         const std::vector<MockFile>& imports,
                         Ds&&... expected_diagnostics) -> void {
    auto [ctx, idx] = collect(failing, imports);
    check_errors_against<sema::Diagnostics>(ctx->root_mod,
                                            std::forward<Ds>(expected_diagnostics)...);
}

// Tests a semantically failing input against the expected generated errors
template <std::same_as<sema::Diagnostic>... Ds>
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    test_collector_fail(failing, {}, std::forward<Ds>(expected_diagnostics)...);
}

// Walk through the iterable and find the first matching expression statement
template <traits::ASTNode Node, typename NodeIterable>
[[nodiscard]] auto lookup_expression(const NodeIterable& nodes, const mod::Module& module) noexcept
    -> const Node& {
    for (const auto& body_id : nodes) {
        if (const auto& expr_stmt = module.ast.get_as_opt<ast::ExpressionStatement>(body_id)) {
            if (expr_stmt->expression.template is<ast::CallExpression>()) {
                return helpers::unwrap(
                    module.ast.get_as_opt<ast::CallExpression>(expr_stmt->expression));
            }
        }
    }
    FAIL("Call expression could not be found");
}

// Returns the context for optional poison checking
template <std::same_as<sema::Diagnostic>... Ds>
auto test_resolver_fail(std::string_view             failing,
                        const std::vector<MockFile>& imports,
                        Ds&&... expected_diagnostics) -> CtxIdxPair {
    auto [ctx, idx] = resolve(failing, imports);
    check_errors_against<sema::Diagnostics>(ctx->root_mod,
                                            std::forward<Ds>(expected_diagnostics)...);
    return {std::move(ctx), idx};
}

// Returns the context for optional poison checking
template <std::same_as<sema::Diagnostic>... Ds>
auto test_resolver_fail(std::string_view failing, Ds&&... expected_diagnostics) {
    return test_resolver_fail(failing, {}, std::forward<Ds>(expected_diagnostics)...);
}

} // namespace ghoti::tests::helpers
