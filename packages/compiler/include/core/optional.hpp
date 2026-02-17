#pragma once

#include <memory>
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

namespace optional {

// Compares two values, forwarding safety concerns to the comparator.
template <typename T>
auto safe_eq(const Optional<T>& a, const Optional<T>& b, bool (*cmp)(const T&, const T&)) noexcept
    -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(*a, *b);
}

// Compares two unique pointers by assuming that both are valid.
template <typename T>
auto unsafe_eq(const Optional<std::unique_ptr<T>>& a,
               const Optional<std::unique_ptr<T>>& b,
               bool (*cmp)(const T&, const T&)) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(**a, **b);
}

// Compares two unique pointers by assuming that both are valid, using the default equality
// operator.
template <typename T>
auto unsafe_eq(const Optional<std::unique_ptr<T>>& a,
               const Optional<std::unique_ptr<T>>& b) noexcept -> bool {
    return unsafe_eq<T>(a, b, [](const T& ae, const T& be) { return ae == be; });
}

} // namespace optional

} // namespace conch
