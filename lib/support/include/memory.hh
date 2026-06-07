#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "assert.hh"

namespace ghoti::mem {

// cppcheck-suppress-begin noExplicitConstructor

// An alias for a unique pointer which should be seldom used
template <typename T, typename D = std::default_delete<T>>
using NullableBox = std::unique_ptr<T, D>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_nullable_box(Args&&... args) -> NullableBox<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// A light unique pointer wrapper that ensures pointer validity at initialization.
//
// All methods besides constructors and factories ASSERT this invariant.
template <typename T, typename D = std::default_delete<T>> class Box {
  public:
    explicit Box(NullableBox<T, D>&& ptr) : ptr_{std::move(ptr)} {
        ASSERT(ptr_, "Box cannot be created from nullptr");
    }

    template <typename P> explicit Box(P* ptr) : ptr_{NullableBox<T>{static_cast<T*>(ptr)}} {
        ASSERT(ptr_, "Box cannot be created from nullptr");
    }

    template <typename U, typename E>
        requires(std::is_convertible_v<U*, T*>)
    Box(Box<U, E>&& other) : ptr_{std::move(other.ptr_)} {}

    ~Box()                             = default;
    Box(const Box&)                    = delete;
    auto operator=(const Box&) -> Box& = delete;
    Box(Box&&)                         = default;
    auto operator=(Box&&) -> Box&      = default;

    [[nodiscard]] auto operator*() const noexcept -> T& { return *get(); }
    [[nodiscard]] auto operator->() const noexcept -> T* { return get(); }

    [[nodiscard]] auto release() noexcept -> T* {
        ASSERT(ptr_, "Attempted to release a moved-from Box");
        return ptr_.release();
    }

    [[nodiscard]] auto get() const noexcept -> T* {
        ASSERT(ptr_, "Attempted to access a moved-from Box");
        return ptr_.get();
    }

    template <typename... Args> [[nodiscard]] static auto make(Args&&... args) -> Box<T> {
        return Box{make_nullable_box<T>(std::forward<Args>(args)...)};
    }

    operator bool() const noexcept { return ptr_.operator bool(); }

  private:
    NullableBox<T, D> ptr_;

    template <typename U, typename E> friend class Box;
};

template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_box(Args&&... args) -> Box<T> {
    return Box<T>::make(std::forward<Args>(args)...);
}

// If you find yourself using this, think really hard about the decisions that led you here...
template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// cppcheck-suppress-end noExplicitConstructor

} // namespace ghoti::mem
