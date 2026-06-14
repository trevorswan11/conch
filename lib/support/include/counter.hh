#pragma once

#include <concepts>

#include "type_traits.hh"
#include "types.hh"

namespace ghoti {

// A simple counter that with RAII-based up/down counting
template <traits::Integral Underlying> class Counter {
  public:
    class Guard {
      public:
        constexpr explicit Guard(Counter& c) : c_{&c} { c_->increment(); }
        ~Guard() {
            if (c_) { c_->decrement(); }
        }

        Guard(const Guard&)                    = delete;
        auto operator=(const Guard&) -> Guard& = delete;

        Guard(Guard&& other) noexcept : c_{other.c_} { other.c_ = nullptr; }
        auto operator=(const Guard&&) -> Guard& = delete;

      private:
        Counter* c_;
    };

  public:
    constexpr auto increment() noexcept -> void { count_ += static_cast<Underlying>(1); }
    constexpr auto decrement() noexcept -> void { count_ -= static_cast<Underlying>(1); }

    constexpr operator bool() noexcept { return count_ != static_cast<Underlying>(0); }
    constexpr operator Underlying() noexcept { return count_; }

    constexpr auto               operator<=>(const Counter&) const noexcept        = default;
    [[nodiscard]] constexpr auto operator==(const Counter&) const noexcept -> bool = default;

    template <std::convertible_to<Underlying> T>
    constexpr auto operator<=>(const T& other) const noexcept {
        return count_ <=> static_cast<Underlying>(other);
    }

    template <std::convertible_to<Underlying> T>
    [[nodiscard]] constexpr auto operator==(const T& other) const noexcept -> bool {
        return count_ == static_cast<Underlying>(other);
    }

    // Creates an RAII incrementor/decrementor object
    [[nodiscard]] constexpr auto guard() noexcept -> Guard { return Guard{*this}; }

  private:
    Underlying count_{static_cast<Underlying>(0)};
};

using DefaultCounter = Counter<usize>;

} // namespace ghoti
