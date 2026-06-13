#pragma once

#include <concepts>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>
#include <gsl/span>
#include <magic_enum/magic_enum.hpp>

#include "iterator.hh"
#include "option.hh"
#include "style.hh"
#include "type_traits.hh"
#include "types.hh"
#include "utility.hh"

namespace ghoti {

// Should be zero indexed and only 1-indexed at print time
struct SourceLocation {
    usize line{0};
    usize column{0};

    SourceLocation() noexcept = default;
    SourceLocation(usize line, usize column) noexcept : line{line}, column{column} {}

    auto operator==(const SourceLocation& other) const noexcept -> bool {
        return line == other.line && column == other.column;
    }
};

namespace traits {

template <typename T> struct SourceInfo;

template <typename T>
concept Locateable = requires(T t) {
    { SourceInfo<T>::get(t) } -> std::same_as<SourceLocation>;
};

template <> struct SourceInfo<std::pair<usize, usize>> {
    static auto get(const std::pair<usize, usize>& p) noexcept -> SourceLocation {
        return {p.first, p.second};
    }
};

template <> struct SourceInfo<SourceLocation> {
    static auto get(const SourceLocation& loc) -> SourceLocation { return loc; }
};

} // namespace traits

namespace mod { struct Module; } // namespace mod

enum class DiagnosticLevel : u8 {
    ERROR,
    WARNING,
};

namespace detail {

// Returns the level fit for diagnostic printing
[[nodiscard]] constexpr auto level_name(DiagnosticLevel level) noexcept -> std::string_view {
    switch (level) {
    case DiagnosticLevel::ERROR:   return "error";
    case DiagnosticLevel::WARNING: return "warning";
    default:                       return "";
    }
}

// Returns the level's style for diagnostic printing
[[nodiscard]] constexpr auto level_style(DiagnosticLevel level) noexcept {
    switch (level) {
    case DiagnosticLevel::ERROR:   return style::RED;
    case DiagnosticLevel::WARNING: return style::LIGHT_YELLOW;
    default:                       return style::BASE;
    }
}

// A decomposed diagnostic that contains all information for base formatting
struct FormattableDiagnostic {
    const opt::Option<std::string>&     message;
    const opt::Option<SourceLocation>&  location;
    std::string_view                    error_name;
    const opt::Option<DiagnosticLevel>& level;
};

auto format_diagnostic(std::ostream&                   os,
                       const FormattableDiagnostic&    diag,
                       const opt::Option<std::string>& source_path,
                       opt::Option<bool>               in_terminal) -> std::ostream&;

} // namespace detail

template <traits::ScopedEnum E> class Diagnostic {
  public:
    explicit Diagnostic(E err) noexcept : error_{err} {}
    Diagnostic(E err, usize line, usize column) noexcept : loc_{{line, column}}, error_{err} {}
    Diagnostic(opt::Option<std::string> msg, E err, usize line, usize column) noexcept
        : message_{std::move(msg)}, loc_{{line, column}}, error_{err} {}
    Diagnostic(opt::Option<std::string> msg, E err) noexcept
        : message_{std::move(msg)}, error_{err} {}

    template <traits::Locateable T>
    Diagnostic(opt::Option<std::string> msg, E err, const T& t) noexcept
        : message_{std::move(msg)}, loc_{traits::SourceInfo<T>::get(t)}, error_{err} {}

    template <traits::Locateable T>
    Diagnostic(E err, T t) : loc_{traits::SourceInfo<T>::get(t)}, error_{err} {}

    // Moves the passed diagnostic into a new one with an error code
    Diagnostic(Diagnostic&& other, E err) noexcept
        : message_{std::move(other.message_)}, loc_{other.loc_}, error_{err} {}

    // Moves the passed diagnostic into a new one with a specified source location
    template <traits::Locateable T>
    Diagnostic(Diagnostic&& other, const T& t) noexcept
        : message_{std::move(other.message_)}, loc_{traits::SourceInfo<T>::get(t)},
          error_{other.error_} {}

    [[nodiscard]] auto to_string(const opt::Option<std::string>& source_path = opt::none,
                                 opt::Option<bool> in_terminal = opt::none) const -> std::string {
        std::stringstream ss;
        detail::format_diagnostic(ss, to_formattable(), source_path, in_terminal);
        return ss.str();
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && loc_ == other.loc_ && error_ == other.error_ &&
               level_ == other.level_;
    }

    MAKE_GETTER(message, const opt::Option<std::string>&)
    [[nodiscard]] auto to_formattable() const noexcept -> detail::FormattableDiagnostic {
        return {message_, loc_, magic_enum::enum_name(error_), level_};
    }

    // Diagnostics are always ERROR by default, see `unset_level`
    auto set_level(DiagnosticLevel level) noexcept -> void { level_.emplace(level); }
    auto unset_level() noexcept -> void { level_.reset(); }

  private:
    opt::Option<std::string>     message_;
    opt::Option<SourceLocation>  loc_;
    E                            error_;
    opt::Option<DiagnosticLevel> level_{DiagnosticLevel::ERROR};
};

namespace traits {

template <typename T> struct is_diagnostic : std::false_type {};
template <typename T> struct is_diagnostic<Diagnostic<T>> : std::true_type {};

template <typename T>
concept DiagnosticType = is_diagnostic<T>::value;

} // namespace traits

template <traits::DiagnosticType D> class DiagnosticList {
  public:
    using value_type = D;
    MAKE_ITERATOR(Diagnostics, std::vector<D>, diagnostics_) // cppcheck-suppress syntaxError

  public:
    explicit DiagnosticList(opt::Option<bool> in_terminal = opt::none) noexcept
        : in_terminal_{in_terminal} {}
    ~DiagnosticList() = default;

    MAKE_MOVE_ONLY(DiagnosticList)

    auto push_back(const D& d) -> void { diagnostics_.push_back(d); }

    template <typename... Args> auto emplace_back(Args&&... args) -> void {
        diagnostics_.emplace_back(std::forward<Args>(args)...);
    }

    operator gsl::span<const D>() const { return diagnostics_; }

    // Creates a new list with the same terminal behavior
    [[nodiscard]] auto create_new() const -> DiagnosticList { return DiagnosticList{in_terminal_}; }
    [[nodiscard]] auto get_terminal_status() const noexcept -> opt::Option<bool> {
        return in_terminal_;
    }

  private:
    Diagnostics       diagnostics_;
    opt::Option<bool> in_terminal_;
};

} // namespace ghoti

template <> struct fmt::formatter<ghoti::SourceLocation> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    static auto format(const ghoti::SourceLocation& loc, format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}", loc.line + 1, loc.column + 1);
    }
};

template <typename E> struct fmt::formatter<ghoti::Diagnostic<E>> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    static auto format(const ghoti::Diagnostic<E>& d, format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};
