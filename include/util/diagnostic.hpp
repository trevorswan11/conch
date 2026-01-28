#pragma once

#include <format>
#include <type_traits>
#include <utility>

#include "util/common.hpp"
#include "util/optional.hpp"

template <typename E>
    requires std::is_scoped_enum_v<E>
class Diagnostic {
  public:
    Diagnostic() = delete;
    explicit Diagnostic(E err) : error_{err} {}
    explicit Diagnostic(E err, usize ln, usize col) : error_{err}, line_{ln}, column_{col} {}
    explicit Diagnostic(std::string msg, E err, usize ln, usize col)
        : message_{std::move(msg)}, error_{err}, line_{ln}, column_{col} {}

    template <typename T>
        requires requires(T t) {
            t.line;
            t.column;
        }
    explicit Diagnostic(std::string msg, E err, T t)
        : message_{std::move(msg)}, error_{err}, line_{t.line}, column_{t.column} {}

    template <typename T>
        requires requires(T t) {
            t.line;
            t.column;
        }
    explicit Diagnostic(E err, T t) : error_{err}, line_{t.line}, column_{t.column} {}

    explicit Diagnostic(Diagnostic& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, line_{other.line_},
          column_{other.column_} {}

    auto message() const noexcept -> const std::string& { return message_; }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && line_ == other.line_ &&
               column_ == other.column_;
    }

  private:
    std::string     message_{};
    E               error_;
    Optional<usize> line_{};
    Optional<usize> column_{};

    friend struct std::formatter<Diagnostic>;
};

template <typename E> struct std::formatter<Diagnostic<E>> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const Diagnostic<E>& d, F& ctx) const {
        if (d.line_ && d.column_) {
            return std::formatter<std::string>::format(
                std::format("{} [{}, {}]",
                            d.message_.empty() ? enum_name(d.error_) : d.message_,
                            *d.line_,
                            *d.column_),
                ctx);
        }

        return std::formatter<std::string>::format(
            std::format("{}", d.message_.empty() ? enum_name(d.error_) : d.message_), ctx);
    }
};
