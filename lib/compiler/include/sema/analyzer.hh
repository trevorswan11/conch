#pragma once

#include <filesystem>
#include <ostream>

#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include <option.hh>
#include <result.hh>
#include <types.hh>
#include <utility.hh>

namespace ghoti::sema {

// The manager for all steps of semantic analysis.
class Analyzer {
  public:
    explicit Analyzer(mod::ModuleManager& modules,
                      std::ostream&       error_stream,
                      opt::Option<bool>   in_terminal) noexcept
        : modules_{modules}, error_stream_{error_stream}, in_terminal_{in_terminal},
          ctx_{modules_, registry_, pool_, Diagnostics{in_terminal_}, error_stream_} {}
    ~Analyzer() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Analyzer)

    // Runs the entire sema pipeline
    auto analyze(const std::filesystem::path& entry_path) -> Result<void, Diagnostic>;

    [[nodiscard]] auto get_table(this auto&& self, usize idx) -> auto& {
        return self.registry_.get(idx);
    }

    [[nodiscard]] auto get_table_opt(this auto&& self, usize idx) noexcept {
        return self.registry_.get_opt(idx);
    }

    MAKE_DEDUCING_GETTER(registry)
    MAKE_DEDUCING_GETTER(pool)

    [[nodiscard]] auto get_prelude_index_opt() const noexcept -> opt::Size {
        return ctx_.prelude_index;
    }

    auto collect_symbols(mod::Module& module) -> mod::ModuleState;
    auto resolve_types(mod::Module& module) -> mod::ModuleState;

  private:
    mod::ModuleManager& modules_;
    SymbolTableRegistry registry_;
    TypePool            pool_;
    std::ostream&       error_stream_;
    opt::Option<bool>   in_terminal_;

    Context ctx_;
};

} // namespace ghoti::sema
