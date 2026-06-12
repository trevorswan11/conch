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

namespace detail {

template <typename...> struct unique_types : std::true_type {};

template <typename T, typename... Rest>
struct unique_types<T, Rest...>
    : std::bool_constant<(!(std::is_same_v<T, Rest> || ...)) && unique_types<Rest...>::value> {};

template <typename... Ts> constexpr auto unique_types_v = unique_types<Ts...>::value;

} // namespace detail

namespace traits {

template <typename... Ts>
concept UniqueTypes = detail::unique_types_v<Ts...>;

} // namespace traits

// Non-constexpr capable yet efficient (compilation performance) `std::variant` alternative
//
// This is not compatible with the standard's definition of variant.
//
// You might argue that I should just use `std::variant` because this is just a bug waiting to
// happen, but I would say that the gains from doing this outweigh the headaches of a few bugs down
// the line. The adoption of this data structure brought the size of the compiler's static library
// (which relies heavily on variants and `std::visit`) from 173M to 58M on macOS (in debug mode).
//
// If I catch any usage of `std::variant` I will lose my marbles...
//
// Inspired by: https://github.com/groundswellaudio/swl-variant
template <typename... Ts>
    requires(sizeof...(Ts) > 0 && traits::UniqueTypes<Ts...>)
class Variant {
  public:
    static constexpr auto N{sizeof...(Ts)};
    using index_type = traits::min_uint_for_bits<min_bits<N>>;

  private:
    template <usize I> using nth = __type_pack_element<I, Ts...>;
    template <typename T>
    static constexpr usize index_of = [] -> usize {
        usize i{0};
        bool  found{(... || (std::is_same_v<std::remove_cvref_t<T>, Ts> ? true : (++i, false)))};
        return found ? i : N;
    }();

    template <typename T, typename... Args>
    static constexpr auto nothrow_constructable{
        traits::NoThrowConstructible<std::remove_cvref_t<T>, Args...>};
    static constexpr auto nothrow_copy{(traits::NoThrowCopyConstructible<Ts> && ...)};
    static constexpr auto nothrow_move{(traits::NoThrowMoveConstructible<Ts> && ...)};

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
    Variant(T&& t) noexcept(nothrow_constructable<T, T&&>) {
        using U = std::remove_cvref_t<T>;
        ::new (storage_) U{std::forward<T>(t)};
        index_ = static_cast<index_type>(index_of<T>);
    }

    // In-place construction
    template <typename T, typename... Args>
        requires(index_of<T> < N && traits::Constructible<T, Args && ...>)
    explicit Variant(std::in_place_type_t<T>,
                     Args&&... args) noexcept(nothrow_constructable<T, Args&&...>) {
        ::new (storage_) T{std::forward<Args>(args)...};
        index_ = static_cast<index_type>(index_of<T>);
    }
    // cppcheck-suppress-end noExplicitConstructor

    ~Variant()
        requires(traits::TriviallyDestructible<Ts> && ...)
    = default;
    ~Variant() { destroy_active(); }

    // NOLINTBEGIN
    Variant(const Variant& other) noexcept(nothrow_copy) { copy_construct(other); }
    auto operator=(const Variant& other) noexcept(nothrow_copy) -> Variant& {
        return copy_assign(other);
    }

    Variant(Variant&& other) noexcept(nothrow_move) { move_construct(std::move(other)); }
    auto operator=(Variant&& other) noexcept(nothrow_move) -> Variant& {
        return move_assign(std::move(other));
    }
    // NOLINTEND

    [[nodiscard]] auto                       index() const noexcept -> usize { return index_; }
    template <typename T> [[nodiscard]] auto is() const noexcept -> bool {
        return index_ == static_cast<index_type>(index_of<T>);
    }

    // Asserts that the requested type is currently active
    template <typename T, typename Self>
    [[nodiscard]] auto as(this Self&& self) noexcept -> decltype(auto) {
        ASSERT(self.template is<T>(), "Variant::as<T> called on inactive alternative");
        if constexpr (traits::RValueReference<Self>) {
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
        requires(index_of<T> < N && traits::Constructible<T, Args && ...>)
    auto emplace(Args&&... args) noexcept(nothrow_constructable<T, Args&&...>) -> T& {
        destroy_active();
        T* p   = ::new (storage_) T{std::forward<Args>(args)...};
        index_ = static_cast<index_type>(index_of<T>);
        return *p;
    }

    // If the copy throws the Variant is left uninitialized
    template <typename> auto emplace(const Variant& other) noexcept(nothrow_copy) -> Variant& {
        return copy_assign(other);
    }

    // If the move throws the Variant is left uninitialized
    template <typename> auto emplace(Variant&& other) noexcept(nothrow_move) -> Variant& {
        return move_assign(std::move(other));
    }

    // Accepts one or more visitors which must cover all possible variant states
    template <typename... Visitors>
    [[nodiscard]] auto visit(this auto&& self, Visitors&&... vis) -> decltype(auto) {
        return visit_impl(self, Overloaded{std::forward<Visitors>(vis)...});
    }

    [[nodiscard]] auto operator==(const Variant& other) const noexcept -> bool {
        if (index_ != other.index_) { return false; }
        auto result{false};
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept -> void {
            (void)(... ||
                   (index_ == Is ? (result = (*as_raw<nth<Is>>() == *other.as_raw<nth<Is>>()), true)
                                 : false));
        }(std::index_sequence_for<Ts...>{});
        return result;
    }

  private:
    // Retrieve an unchecked properly typed pointer into the underlying storage
    template <typename T, typename Self>
    [[nodiscard]] auto as_raw(this Self&& self) noexcept -> auto* {
        // https://en.cppreference.com/cpp/utility/launder
        return std::launder(reinterpret_cast<traits::const_dispatch_t<Self, T>*>(self.storage_));
    }

    auto destroy_active() noexcept -> void {
        if (index_ >= static_cast<index_type>(N)) { return; }
        [this]<usize... Is>(std::index_sequence<Is...>) noexcept -> void {
            (void)(... || (index_ == Is ? (as_raw<nth<Is>>()->~nth<Is>(), true) : false));
        }(std::index_sequence_for<Ts...>{});
    }

    auto copy_construct(const Variant& other) noexcept(nothrow_copy) -> void {
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept(nothrow_copy) -> void {
            (void)(... ||
                   (other.index_ == Is
                        ? (::new (storage_) nth<Is>{*other.as_raw<nth<Is>>()}, index_ = Is, true)
                        : false));
        }(std::index_sequence_for<Ts...>{});
    }

    auto copy_assign(const Variant& other) noexcept(nothrow_copy) -> Variant& {
        if (this != &other) {
            destroy_active();
            copy_construct(other);
        }
        return *this;
    }

    // Also destroys the moved-from object
    auto move_construct(Variant&& other) noexcept(nothrow_move) -> void {
        [&]<usize... Is>(std::index_sequence<Is...>) noexcept(nothrow_move) -> void {
            (void)(... || (other.index_ == Is
                               ? (::new (storage_) nth<Is>{std::move(*other.as_raw<nth<Is>>())},
                                  index_ = Is,
                                  true)
                               : false));
        }(std::index_sequence_for<Ts...>{});
        other.destroy_active();
        other.index_ = static_cast<index_type>(N);
    }

    auto move_assign(Variant&& other) noexcept(nothrow_move) -> Variant& {
        if (this != &other) {
            destroy_active();
            move_construct(std::move(other));
        }
        return *this;
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
    index_type index_;
    alignas(std::max({alignof(Ts)...})) std::byte storage_[std::max({sizeof(Ts)...})];
};

struct Unit {};

constexpr auto operator==(Unit, Unit) noexcept -> bool { return true; }
constexpr auto operator>(Unit, Unit) noexcept -> bool { return false; }
constexpr auto operator<(Unit, Unit) noexcept -> bool { return false; }
constexpr auto operator<=(Unit, Unit) noexcept -> bool { return true; }
constexpr auto operator>=(Unit, Unit) noexcept -> bool { return true; }

} // namespace ghoti
