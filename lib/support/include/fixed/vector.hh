#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <utility>

#include <gsl/span>

#include "assert.hh"
#include "fixed/storage.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::fixed {

// A fixed-size zero-allocation container with a vector-like interface
template <typename Item, usize Capacity> class Vector {
  public:
    using value_type      = Item;
    using size_type       = usize;
    using reference       = Item&;
    using const_reference = const Item&;
    using iterator        = Item*;
    using const_iterator  = const Item*;

  public:
    Vector() = default;
    ~Vector() { clear(); }
    ~Vector()
        requires traits::TriviallyDestructible<Item>
    = default;

    // Constructs the vector in place by emplacing each item into the buffer
    template <typename... Is>
        requires(sizeof...(Is) <= Capacity)
    constexpr explicit Vector(Is&&... items) {
        (..., emplace_back(std::forward<Is>(items)));
    }

    constexpr Vector(const Vector&)
        requires traits::TriviallyCopyable<Item>
    = default;

    constexpr Vector(const Vector& other) {
        if constexpr (traits::TriviallyCopyable<Item>) {
            size_ = other.size_;
            std::copy(other.begin(), other.end(), data());
        } else {
            for (const auto& item : other) { emplace_back(item); }
        }
    }

    constexpr auto operator=(const Vector&) -> Vector&
        requires traits::TriviallyCopyable<Item>
    = default;

    constexpr auto operator=(const Vector& other) -> Vector& {
        if (this != &other) {
            Vector temp{other};
            swap(temp);
        }
        return *this;
    }

    constexpr Vector(Vector&& other) noexcept {
        if constexpr (traits::TriviallyCopyable<Item>) {
            size_ = other.size_;
            std::copy(other.begin(), other.end(), data());
        } else {
            for (auto& item : other) { emplace_back(std::move(item)); }
        }
        other.clear();
    }

    constexpr auto operator=(Vector&& other) noexcept -> Vector& {
        if (this != &other) {
            clear();
            if constexpr (traits::TriviallyCopyable<Item>) {
                size_ = other.size_;
                std::copy(other.begin(), other.end(), data());
            } else {
                for (auto& item : other) { emplace_back(std::move(item)); }
            }
            other.clear();
        }
        return *this;
    }

    // Constructs an object in place at the end of the vector with the provided args
    template <typename... Args> constexpr auto emplace_back(Args&&... args) -> void {
        ASSERT(size_ < Capacity, "StaticVector size out of range");
        std::construct_at(data() + size_, std::forward<Args>(args)...);
        size_++;
    }

    constexpr auto push_back(const Item& item) -> void { emplace_back(item); }
    constexpr auto push_back(Item&& item) -> void { emplace_back(std::move(item)); }

    [[nodiscard]] constexpr explicit operator gsl::span<Item>() noexcept { return {data(), size_}; }
    [[nodiscard]] constexpr explicit operator gsl::span<const Item>() const noexcept {
        return {data(), size_};
    }

    [[nodiscard]] constexpr auto operator[](this auto&& self, usize idx) noexcept
        -> decltype(auto) {
        ASSERT(idx < self.size_, "StaticVector index out of bounds");
        return self.data()[idx];
    }

    [[nodiscard]] constexpr auto begin(this auto&& self) noexcept -> auto* { return self.data(); }
    [[nodiscard]] constexpr auto end(this auto&& self) noexcept -> auto* {
        return self.data() + self.size_;
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return size_; }
    [[nodiscard]] constexpr auto capacity() const noexcept -> usize { return Capacity; }

    [[nodiscard]] constexpr auto data(this auto&& self) noexcept -> auto* {
        return self.items_.data();
    }

    constexpr auto clear() noexcept -> void {
        if constexpr (!traits::TriviallyDestructible<Item>) {
            // The lion is now concerned with freeing non-trivial resources
            for (usize i{0}; i < size_; ++i) { std::destroy_at(data() + i); }
        }
        size_ = 0;
    }

  private:
    // https://en.cppreference.com/cpp/algorithm/swap
    constexpr auto swap(Vector& other) noexcept -> void
        requires(!traits::TriviallyCopyable<Item>)
    {
        auto& smaller{(size_ < other.size_) ? *this : other};
        auto& larger{(size_ < other.size_) ? other : *this};

        std::swap_ranges(smaller.begin(), smaller.end(), larger.begin());
        const auto smaller_size{smaller.size_};
        const auto larger_size{larger.size_};

        // Manually destroy the moved-from object after moving it
        for (usize i{smaller_size}; i < larger_size; ++i) {
            smaller.emplace_back(std::move(larger[i]));
            std::destroy_at(data() + i);
        }

        smaller.size_ = larger_size;
        larger.size_  = smaller_size;
    }

  private:
    detail::Storage<Item, Capacity> items_;
    usize                           size_{0};
};

} // namespace ghoti::fixed
