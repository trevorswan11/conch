#pragma once

#include <format>
#include <type_traits>
#include <utility>

#include "core/common.hpp"
#include "core/optional.hpp"
#include "core/source_loc.hpp"

namespace conch {

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

    auto message() const noexcept -> const std::string& { return message_; }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

  private:
    std::string              message_{};
    E                        error_;
    Optional<SourceLocation> loc_{};

    friend struct std::formatter<Diagnostic>;
};

} // namespace conch

template <typename E> struct std::formatter<conch::Diagnostic<E>> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const conch::Diagnostic<E>& d, F& ctx) const {
        if (d.loc_) {
            return std::formatter<std::string>::format(
                std::format("{} [{}, {}]",
                            d.message_.empty() ? enum_name(d.error_) : d.message_,
                            d.loc_->line,
                            d.loc_->column),
                ctx);
        }

        return std::formatter<std::string>::format(
            std::format("{}", d.message_.empty() ? enum_name(d.error_) : d.message_), ctx);
    }
};
