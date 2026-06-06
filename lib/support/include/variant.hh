#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include "assert.hh"
#include "math.hh"
#include "option.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti {

// https://en.cppreference.com/cpp/utility/variant/visit2
template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Non-constexpr capable yet efficient (compilation performance) `std::variant` replacement
//
// You might argue that I should just use `std::variant` because this is just a bug waiting to
// happen, but I would say that the gains from doing this outweigh the headaches of a few bugs down
// the line. The adoption of this data structure brought the size of the compiler's static library
// (which relies heavily on variants and `std::visit`) from 173M to 58M on macOS (in debug mode).
//
// If I catch any usage of `std::variant` I will lose my marbles...
template <typename... Ts> class Variant {
  public:
    static constexpr auto N = sizeof...(Ts);
    using index_type        = traits::min_uint_for_bits<N>;

  private:
    // Inspired by: https://github.com/groundswellaudio/swl-variant
    template <usize I> using nth = __type_pack_element<I, Ts...>;
    template <typename T>
    static constexpr usize index_of = [] {
        usize i     = 0;
        bool  found = (... || (std::is_same_v<std::remove_cvref_t<T>, Ts> ? true : (++i, false)));
        return found ? i : N;
    }();

    template <typename T, typename... Args>
    static constexpr auto nothrow_construct =
        traits::NoThrowConstructible<std::remove_cvref_t<T>, Args...>;
    static constexpr auto nothrow_copy = (traits::NoThrowMoveConstructible<Ts> && ...);
    static constexpr auto nothrow_move = (traits::NoThrowCopyConstructible<Ts> && ...);

  public:
    // cppcheck-suppress-begin noExplicitConstructor

    // Value-initialize the first alternative
    Variant()
        requires traits::DefaultConstructible<nth<0>>
        : index_{0} {
        ::new (storage_) nth<0>{};
    }

    // Construct from any alternative type
    template <typename T>
        requires(index_of<T> < N)
    Variant(T&& t) noexcept(nothrow_construct<T, T&&>) {
        using U = std::remove_cvref_t<T>;
        ::new (storage_) U{std::forward<T>(t)};
        index_ = static_cast<index_type>(index_of<T>);
    }

    // In-place construction
    template <typename T, typename... Args>
        requires(index_of<T> < N && std::is_constructible_v<T, Args && ...>)
    explicit Variant(std::in_place_type_t<T>,
                     Args&&... args) noexcept(nothrow_construct<T, Args...>) {
        ::new (storage_) T{std::forward<Args>(args)...};
        index_ = static_cast<index_type>(index_of<T>);
    }
    // cppcheck-suppress-end noExplicitConstructor

    ~Variant()
        requires(traits::TriviallyDestructible<Ts> && ...)
    = default;
    ~Variant() { destroy_active(); }

    Variant(const Variant& other) noexcept(nothrow_copy) { copy_construct(other); }
    auto operator=(const Variant& other) noexcept(nothrow_copy) -> Variant& {
        if (this != &other) {
            destroy_active();
            copy_construct(other);
        }
        return *this;
    }

    Variant(Variant&& other) noexcept(nothrow_move) { move_construct(std::move(other)); }
    auto operator=(Variant&& other) noexcept(nothrow_move) -> Variant& {
        if (this != &other) { move_construct(std::move(other)); }
        return *this;
    }

    [[nodiscard]] auto                       index() const noexcept -> usize { return index_; }
    template <typename T> [[nodiscard]] auto is() const noexcept -> bool {
        return index_ == static_cast<index_type>(index_of<T>);
    }

    // Asserts that the requested type is currently active
    template <typename T, typename Self> [[nodiscard]] auto as(this Self&& self) -> decltype(auto) {
        ASSERT(self.template is<T>(), "Variant::as<T> called on inactive alternative");
        if constexpr (std::is_rvalue_reference_v<Self>) {
            return std::move(*self.template as_raw<T>());
        } else {
            return *self.template as_raw<T>();
        }
    }

    // Returns a reference to the active type if T matches
    template <typename T, typename Self>
    [[nodiscard]] auto as_opt(this Self& self) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, T>&> {
        if (!self.template is<T>()) { return opt::none; }
        return {self.template as_raw<T>()};
    }

    // Safely cleans up the active type before constructing a new type in place
    template <typename T, typename... Args>
        requires(index_of<T> < N && std::is_constructible_v<T, Args && ...>)
    auto emplace(Args&&... args) noexcept(nothrow_construct<T, Args...>) -> T& {
        destroy_active();
        T* p   = ::new (storage_) T{std::forward<Args>(args)...};
        index_ = static_cast<index_type>(index_of<T>);
        return *p;
    }

    // Identical behavior to copy construction
    template <typename> auto emplace(const Variant& other) noexcept(nothrow_copy) -> Variant& {
        if (this != &other) {
            destroy_active();
            copy_construct(other);
        }
        return *this;
    }

    // Identical behavior to move construction
    template <typename> auto emplace(Variant&& other) noexcept(nothrow_move) -> Variant& {
        if (this != &other) {
            destroy_active();
            move_construct(std::move(other));
        }
        return *this;
    }

    // Forwards all visitors through the `Overloaded` pattern to match on the active type
    template <typename... Visitors>
    [[nodiscard]] auto visit(this auto&& self, Visitors&&... vis) -> decltype(auto) {
        return visit_impl(self, Overloaded{std::forward<Visitors>(vis)...});
    }

    [[nodiscard]] auto operator==(const Variant& other) const noexcept -> bool {
        if (index_ != other.index_) { return false; }
        bool result = false;
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept {
            (void)(... ||
                   (index_ == Is ? (result = (*as_raw<nth<Is>>() == *other.as_raw<nth<Is>>()), true)
                                 : false));
        }(std::index_sequence_for<Ts...>{});
        return result;
    }

  private:
    // Retrieve a properly typed pointer into the underlying storage
    template <typename T, typename Self>
    [[nodiscard]] auto as_raw(this Self&& self) noexcept -> auto* {
        return std::launder(reinterpret_cast<traits::const_dispatch_t<Self, T>*>(self.storage_));
    }

    void destroy_active() noexcept {
        if (index_ >= static_cast<index_type>(N)) { return; }
        // Walk the integer sequence until a destructor is called
        [this]<usize... Is>(std::index_sequence<Is...>) noexcept {
            (void)(... || (index_ == Is ? (as_raw<nth<Is>>()->~nth<Is>(), true) : false));
        }(std::index_sequence_for<Ts...>{});
    }

    void copy_construct(const Variant& other) noexcept(nothrow_copy) {
        // Walk the integer sequence until a constructor is called
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept(nothrow_copy) {
            (void)(... ||
                   (other.index_ == Is
                        ? (::new (storage_) nth<Is>{*other.as_raw<nth<Is>>()}, index_ = Is, true)
                        : false));
        }(std::index_sequence_for<Ts...>{});
    }

    // Also destroys the moved-from object
    void move_construct(Variant&& other) noexcept(nothrow_move) {
        // Walk the integer sequence until a constructor is called
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept(nothrow_move) {
            (void)(... || (other.index_ == Is
                               ? (::new (storage_) nth<Is>{std::move(*other.as_raw<nth<Is>>())},
                                  index_ = Is,
                                  true)
                               : false));
        }(std::index_sequence_for<Ts...>{});
        other.destroy_active();
        other.index_ = static_cast<index_type>(N);
    }

    template <usize I = 0, typename Self, typename Visitor>
    static auto visit_impl(Self&& self, Visitor&& vis)
        -> decltype(std::forward<Visitor>(vis)(*self.template as_raw<nth<0>>())) {
        if constexpr (I < N) {
            if (self.index_ == static_cast<index_type>(I)) {
                return std::forward<Visitor>(vis)(*self.template as_raw<nth<I>>());
            }
            return visit_impl<I + 1>(std::forward<Self>(self), std::forward<Visitor>(vis));
        }
        UNREACHABLE("Active index out of range");
    }

  private:
    alignas(std::max({alignof(Ts)...})) std::byte storage_[std::max({sizeof(Ts)...})];
    index_type index_;
};

struct Unit {};

constexpr auto operator==(Unit, Unit) noexcept -> bool { return true; }
constexpr auto operator>(Unit, Unit) noexcept -> bool { return false; }
constexpr auto operator<(Unit, Unit) noexcept -> bool { return false; }
constexpr auto operator<=(Unit, Unit) noexcept -> bool { return true; }
constexpr auto operator>=(Unit, Unit) noexcept -> bool { return true; }

} // namespace ghoti
