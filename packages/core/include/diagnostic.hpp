#pragma once

#include <format>
#include <optional>
#include <type_traits>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include "source_loc.hpp"
#include "types.hpp"

namespace conch {

namespace detail {

[[nodiscard]] auto format_diagnostic(const std::optional<std::string>&    message,
                                     std::string_view                     error_name,
                                     const std::optional<SourceLocation>& location) -> std::string;

} // namespace detail

template <typename E>
    requires std::is_scoped_enum_v<E>
class Diagnostic {
  public:
    Diagnostic() = delete;
    explicit Diagnostic(E err) : error_{err} {}
    explicit Diagnostic(E err, usize line, usize column)
        : error_{err}, loc_{SourceLocation{line, column}} {}
    explicit Diagnostic(std::string msg, E err, usize line, usize column)
        : message_{std::move(msg)}, error_{err}, loc_{SourceLocation{line, column}} {}

    template <Locateable T>
    explicit Diagnostic(std::string msg, E err, const T& t)
        : message_{std::move(msg)}, error_{err}, loc_{SourceInfo<T>::get(t)} {}

    template <Locateable T>
    explicit Diagnostic(E err, T t) : error_{err}, loc_{SourceInfo<T>::get(t)} {}

    explicit Diagnostic(Diagnostic& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, loc_{std::move(other.loc_)} {}

    auto has_msg() const noexcept -> bool { return message_.has_value(); }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    [[nodiscard]] auto to_string() const noexcept -> std::string {
        return detail::format_diagnostic(message_, magic_enum::enum_name(error_), loc_);
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

  private:
    std::optional<std::string>    message_{};
    E                             error_;
    std::optional<SourceLocation> loc_{};

    friend struct std::formatter<Diagnostic>;
};

} // namespace conch

template <typename E> struct std::formatter<conch::Diagnostic<E>> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const conch::Diagnostic<E>& d, F& ctx) const {
        return std::formatter<std::string>::format(d.to_string(), ctx);
    }
};
