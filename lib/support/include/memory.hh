#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "assert.hh"
#include "option.hh"

namespace ghoti {

namespace mem {

// cppcheck-suppress-begin noExplicitConstructor

// An alias for a unique pointer which should be seldom used
template <typename T, typename D = std::default_delete<T>>
using NullableBox = std::unique_ptr<T, D>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_nullable_box(Args&&... args) -> NullableBox<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Makes a new nullable box from an existing pointer
template <typename T, typename P>
[[nodiscard]] constexpr auto nullable_box_from(P* ptr) -> NullableBox<T> {
    return std::unique_ptr<T>{static_cast<T*>(ptr)};
}

// Makes a new nullable box from an existing box, changing the type as requested
template <typename T, typename P>
[[nodiscard]] constexpr auto nullable_box_into(NullableBox<P>&& ptr) -> NullableBox<T> {
    return std::unique_ptr<T>{static_cast<T*>(ptr.release())};
}

#define MAKE_NULLABLE_BOX_UNPACKER(name, ReturnType, member, deref)                              \
    [[nodiscard]] auto get_##name() const noexcept -> const ReturnType& { return deref member; } \
    [[nodiscard]] auto has_##name() const noexcept -> bool { return member.operator bool(); }

struct NullBoxException : public std::logic_error {
    NullBoxException()
        : std::logic_error{"Violated Invariant: Attempted to initialize with null"} {}
    explicit NullBoxException(std::string message) : std::logic_error{std::move(message)} {}
};

// A light unique pointer wrapper that ensures pointer validity at initialization.
//
// All methods besides constructors and factories ASSERT this invariant.
template <typename T, typename D = std::default_delete<T>> class Box {
  public:
    explicit Box(NullableBox<T, D>&& ptr) : ptr_{std::move(ptr)} {
        if (!ptr_) { throw NullBoxException{}; }
    }

    template <typename P> explicit Box(P* ptr) : ptr_{NullableBox<T>{static_cast<T*>(ptr)}} {
        if (!ptr_) { throw NullBoxException{}; }
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
        return Box{std::make_unique<T>(std::forward<Args>(args)...)};
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

// Makes a new box from an existing pointer
template <typename T, typename P> [[nodiscard]] constexpr auto box_from(P* ptr) -> Box<T> {
    return Box<T>{ptr};
}

// Makes a new box from an existing box, changing the type as requested
template <typename T, typename P> [[nodiscard]] constexpr auto box_into(Box<P>&& ptr) -> Box<T> {
    return Box<T>{ptr.release()};
}

// Makes a new nullable box from an existing box
template <typename T>
[[nodiscard]] constexpr auto nullable_box_from(Box<T>&& box) -> NullableBox<T> {
    return std::unique_ptr<T>{box.release()};
}

// If you find yourself using this, think really hard about the decisions that led you here...
template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// cppcheck-suppress-end noExplicitConstructor

// Compares two boxes based on the provided comparator
template <typename T, typename Comparator>
constexpr auto nullable_boxes_eq(const mem::NullableBox<T>& a,
                                 const mem::NullableBox<T>& b,
                                 Comparator                 cmp) noexcept -> bool {
    if (a.operator bool() != b.operator bool()) { return false; }
    if (!a.operator bool()) { return true; }
    return cmp(*a, *b);
}

// Compares two boxes by using the underlying type's default equality operator
template <typename T>
constexpr auto nullable_boxes_eq(const mem::NullableBox<T>& a,
                                 const mem::NullableBox<T>& b) noexcept -> bool {
    return nullable_boxes_eq<T>(a, b, [](const auto& ae, const auto& be) { return ae == be; });
}

// A non-null pointer for use when a reference is inappropriate.
template <typename T>
    requires(!std::is_reference_v<T>)
class NonNull {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr NonNull(T* ptr) noexcept : ptr_{ptr} {
        ASSERT(ptr_, "Attempt to create NonNull from nullptr");
    }

    constexpr NonNull(T& ref) noexcept : ptr_{&ref} {}

    constexpr NonNull(opt::detail::Ref<T> opt) : ptr_{&opt.value()} {}
    NonNull(opt::None) = delete;
    NonNull(T&&)       = delete;

    template <typename U>
        requires(std::convertible_to<U*, T*>)
    constexpr NonNull(const NonNull<U>& other) noexcept : ptr_{other.get()} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto operator->() const noexcept -> T* { return get(); }
    [[nodiscard]] constexpr auto operator*() const noexcept -> T& { return *get(); }

    [[nodiscard]] constexpr auto get() const noexcept -> T* {
        ASSERT(ptr_, "Attempt to access invalid non-null");
        return ptr_;
    }

    [[nodiscard]] constexpr auto operator==(const NonNull<T>&) const noexcept -> bool = default;

  private:
    // This should seldom be called as it violates an invariant
    NonNull() noexcept = default;

  private:
    T* ptr_;

    friend class Arena;
};

} // namespace mem

namespace traits {

template <typename T> struct is_nullable_box : std::false_type {};
template <typename T, typename D>
struct is_nullable_box<mem::NullableBox<T, D>> : std::true_type {};
template <typename T> constexpr bool is_nullable_box_v = is_nullable_box<T>::value;

template <typename T> struct is_box : std::false_type {};
template <typename T, typename D> struct is_box<mem::Box<T, D>> : std::true_type {};
template <typename T> constexpr bool is_box_v = is_box<T>::value;

template <typename T> using is_ptr            = std::is_pointer<T>;
template <typename T> constexpr auto is_ptr_v = is_ptr<T>::value;

template <typename T>
constexpr auto is_ptr_like_v = is_nullable_box_v<T> || is_box_v<T> || is_ptr_v<T>;

} // namespace traits

template <typename T, typename D> class opt::detail::Ref<mem::Box<T, D>&> {
    static_assert(false, "Use a NullableBox<T, D> to accomplish this!");
};

} // namespace ghoti

template <typename T, typename D> class std::optional<ghoti::mem::Box<T, D>> {
    static_assert(false, "Use a NullableBox<T, D> to accomplish this!");
};
