#pragma once

#include <optional>
#include <type_traits>

namespace conch {

template <typename T> class OptionalRef {
  public:
    OptionalRef() noexcept : ptr_{nullptr} {}
    OptionalRef(std::nullopt_t) noexcept // cppcheck-suppress noExplicitConstructor
        : ptr_{nullptr} {}
    OptionalRef(T& ref) noexcept : ptr_{&ref} {} // cppcheck-suppress noExplicitConstructor

    OptionalRef(T&&)                                            = delete;
    auto operator=(const OptionalRef&) noexcept -> OptionalRef& = default;

    [[nodiscard]] bool     has_value() const noexcept { return ptr_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    auto operator->() const noexcept -> T* { return ptr_; }
    auto operator*() const noexcept -> T& { return *ptr_; }

  private:
    T* ptr_;
};

template <typename T>
using Optional = std::conditional_t<std::is_reference_v<T>,
                                    OptionalRef<std::remove_reference_t<T>>,
                                    std::optional<T>>;

using std::nullopt;

} // namespace conch
