#pragma once

#include <array>
#include <bit>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <gsl/span>

#include "assert.hh"
#include "fixed/storage.hh"
#include "hash.hh"
#include "iterator.hh"
#include "math.hh"
#include "option.hh"
#include "string.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::fixed {

namespace detail {

class Metadata {
  public:
    enum class Fingerprint : u8 {
        OPEN      = 0,
        TOMBSTONE = 1,
    };

  public:
    constexpr Metadata() noexcept = default;
    constexpr Metadata(u8 fingerprint, bool used) noexcept {
        raw_ |= used << USED_OFFSET;
        raw_ |= fingerprint & FINGERPRINT_MASK;
    }

    constexpr Metadata(Fingerprint fingerprint, bool used) noexcept
        : Metadata{static_cast<u8>(fingerprint), used} {}

    [[nodiscard]] static constexpr auto make_open_slot() noexcept -> Metadata {
        return {Fingerprint::OPEN, false};
    }

    [[nodiscard]] static constexpr auto make_tombstone_slot() noexcept -> Metadata {
        return {Fingerprint::TOMBSTONE, false};
    }

    [[nodiscard]] constexpr auto is_used() const noexcept -> bool { return (raw_ & USED_MASK) > 0; }
    [[nodiscard]] constexpr auto get_fingerprint() const noexcept -> u8 {
        return raw_ & FINGERPRINT_MASK;
    }

    constexpr auto               open_up() noexcept -> void { *this = make_open_slot(); }
    [[nodiscard]] constexpr auto is_open() const noexcept -> bool {
        return *this == make_open_slot();
    }

    constexpr auto               bury() noexcept -> void { *this = make_tombstone_slot(); }
    [[nodiscard]] constexpr auto is_tombstone() const noexcept -> bool {
        return *this == make_tombstone_slot();
    }

    // Sets the inner fingerprint and marks the metadata as used
    constexpr auto fill(u8 fingerprint) noexcept -> void { *this = {fingerprint, true}; }

    // Only the 7 most significant bits of the result are relevant
    static constexpr auto take_fingerprint(u64 hash) noexcept -> u8 {
        return FINGERPRINT_MASK & (hash >> (64 - USED_OFFSET));
    }

    [[nodiscard]] constexpr auto operator==(const Metadata&) const noexcept -> bool = default;

  private:
    static constexpr u8 FINGERPRINT_MASK{0x7F};
    static constexpr u8 USED_MASK{0x80};
    static constexpr u8 USED_OFFSET{std::countr_zero(USED_MASK)};

  private:
    u8 raw_{0};
};

// Facilitates const and non-const behavior
template <typename HashMapSelf, typename Key, typename Value, usize Capacity>
class HashMapIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = std::pair<const Key, Value>;
    using difference_type   = idiff;
    using pointer           = void;
    using reference         = std::pair<const Key&, traits::const_dispatch_t<HashMapSelf, Value>&>;

    // Facilitates `->` operator usage without violating memory safety
    struct Proxy {
        reference value;

        [[nodiscard]] constexpr auto operator->() const noexcept -> const reference* {
            return &value;
        }
    };

  public:
    constexpr HashMapIterator() noexcept = default;
    constexpr HashMapIterator(HashMapSelf& hm, usize index) noexcept : hm_{&hm}, index_{index} {
        next();
    }

    // The index is always advanced up to the next occupied slot
    [[nodiscard]] constexpr auto operator++() -> HashMapIterator& {
        if (index_ < Capacity) {
            index_++;
            next();
        }
        return *this;
    }

    [[nodiscard]] constexpr auto operator++(int) -> HashMapIterator {
        HashMapIterator it{*this};
        ++(*this);
        return it;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
        ASSERT(hm_, "Attempt to dereference null hash map");
        return reference{*(hm_->key_data() + index_), *(hm_->value_data() + index_)};
    }

    [[nodiscard]] constexpr auto operator->() const noexcept -> Proxy { return {operator*()}; }

    [[nodiscard]] constexpr auto operator==(const HashMapIterator& other) const noexcept -> bool {
        return hm_ == other.hm_ && index_ == other.index_;
    }

    [[nodiscard]] constexpr auto operator==(std::default_sentinel_t) const noexcept -> bool {
        return index_ >= Capacity;
    }

  private:
    constexpr auto next() noexcept -> void {
        if (!hm_) { return; }
        const auto metadata{hm_->get_metadata()};
        while (index_ < Capacity && !metadata[index_].is_used()) { index_++; }
    }

  private:
    HashMapSelf* hm_{nullptr};
    usize        index_{0};
};

// Heavily inspired by Zig's hash map implementation and trevor's C version:
// https://github.com/trevorswan11/ghoti/blob/4577f3279f5ab09e32a13b8cacb044da686e64bd/src/util/containers/hash_map.c
template <typename Key, typename Value, usize Capacity, typename Hash, typename Equal>
    requires(is_power_of_two(Capacity))
class HashMap {
  public:
    using iterator       = HashMapIterator<HashMap, Key, Value, Capacity>;
    using const_iterator = HashMapIterator<std::add_const_t<HashMap>, Key, Value, Capacity>;

  public:
    constexpr HashMap() noexcept = default;
    constexpr ~HashMap() { clear(); }
    constexpr ~HashMap()
        requires(traits::TriviallyDestructible<Key> && traits::TriviallyDestructible<Value>)
    = default;

    constexpr HashMap(const HashMap&)
        requires(traits::TriviallyCopyable<Key> && traits::TriviallyCopyable<Value>)
    = default;

    constexpr HashMap(const HashMap& other) {
        for (usize i{0}; i < Capacity; ++i) {
            metadata_[i] = other.metadata_[i];
            if (metadata_[i].is_used()) {
                std::construct_at(key_data() + i, *(other.key_data() + i));
                std::construct_at(value_data() + i, *(other.value_data() + i));
            }
        }
        size_ = other.size_;
    }

    constexpr auto operator=(const HashMap&) -> HashMap&
        requires(traits::TriviallyCopyable<Key> && traits::TriviallyCopyable<Value>)
    = default;

    constexpr auto operator=(const HashMap& other) -> HashMap& {
        if (this != &other) {
            HashMap temp{other};
            swap(temp);
        }
        return *this;
    }

    constexpr HashMap(HashMap&& other) noexcept {
        for (usize i{0}; i < Capacity; ++i) {
            metadata_[i] = other.metadata_[i];
            if (metadata_[i].is_used()) {
                std::construct_at(key_data() + i, std::move(*(other.key_data() + i)));
                std::construct_at(value_data() + i, std::move(*(other.value_data() + i)));
            }
        }
        size_ = other.size_;
        other.clear();
    }

    constexpr auto operator=(HashMap&& other) noexcept -> HashMap& {
        if (this != &other) {
            clear();
            HashMap temp{std::move(other)};
            swap(temp);
        }
        return *this;
    }

    [[nodiscard]] constexpr auto get_metadata() const noexcept -> gsl::span<const Metadata> {
        return metadata_;
    }

    // Constructs a value at the key or updates it if there was already an item present
    template <typename... Args> constexpr auto emplace(const Key& key, Args&&... args) -> void {
        const auto hashed{Hash{}(key)};
        const auto fingerprint{Metadata::take_fingerprint(hashed)};

        usize limit{Capacity};
        usize first_tombstone_idx{Capacity};
        usize probe{hashed & HASH_MASK};

        auto m{metadata_[probe]};
        while (!m.is_open() && limit != 0) {
            if (m.is_used() && m.get_fingerprint() == fingerprint) {
                if (Equal{}(key, *(key_data() + probe))) {
                    // Equality doesn't mean all of the key's internals are equal
                    std::destroy_at(key_data() + probe);
                    std::construct_at(key_data() + probe, key);

                    std::destroy_at(value_data() + probe);
                    std::construct_at(value_data() + probe, std::forward<Args>(args)...);
                    return;
                }
            } else if (first_tombstone_idx == Capacity && m.is_tombstone()) {
                first_tombstone_idx = probe;
            }

            limit -= 1;
            probe = (probe + 1) & HASH_MASK;
            m     = metadata_[probe];
        }

        // It's cheaper to lower probing distance after deletions by recycling a tombstone
        if (first_tombstone_idx < Capacity) { probe = first_tombstone_idx; }
        ASSERT(limit != 0, "HashMap is full");
        metadata_[probe].fill(fingerprint);
        size_ += 1;

        std::construct_at(key_data() + probe, key);
        std::construct_at(value_data() + probe, std::forward<Args>(args)...);
    }

    constexpr auto contains(const Key& key) const noexcept -> bool {
        return index_of(key).has_value();
    }

    // Returns a reference to the value at the key if present
    [[nodiscard]] constexpr auto get(this auto&& self, const Key& key) noexcept -> auto& {
        const auto idx{self.index_of(key)};
        ASSERT(idx, "Illegal get on missing key");
        return *(self.value_data() + *idx);
    }

    // Returns a reference to the value at the key or none if the key is not present
    template <typename Self>
    [[nodiscard]] constexpr auto get_opt(this Self&& self, const Key& key) noexcept
        -> opt::Option<traits::const_dispatch_t<Self, Value>&> {
        if (const auto idx{self.index_of(key)}) { return *(self.value_data() + *idx); }
        return opt::none;
    }

    // Removes the key value pair from the map, NOOP if not present
    constexpr auto remove(const Key& key) noexcept -> void {
        const auto idx{index_of(key)};
        if (!idx) { return; }

        std::destroy_at(key_data() + *idx);
        std::destroy_at(value_data() + *idx);

        metadata_[*idx].bury();
        size_ -= 1;
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return size_; }
    [[nodiscard]] constexpr auto capacity() const noexcept -> usize { return Capacity; }

    template <typename Self> [[nodiscard]] constexpr auto begin(this Self&& self) noexcept {
        return HashMapIterator<std::remove_reference_t<Self>, Key, Value, Capacity>{self, 0};
    }

    template <typename Self> [[nodiscard]] constexpr auto end(this Self&& self) noexcept {
        return HashMapIterator<std::remove_reference_t<Self>, Key, Value, Capacity>{self, Capacity};
    }

    [[nodiscard]] constexpr auto key_data(this auto&& self) noexcept -> auto* {
        return self.keys_.data();
    }

    [[nodiscard]] constexpr auto value_data(this auto&& self) noexcept -> auto* {
        return self.values_.data();
    }

    // Destroys all key-value pairs and resets the tracked size
    constexpr auto clear() noexcept -> void {
        if constexpr (!traits::TriviallyDestructible<Key> ||
                      !traits::TriviallyDestructible<Value>) {
            for (usize i{0}; i < Capacity; ++i) {
                if (metadata_[i].is_used()) {
                    if constexpr (!traits::TriviallyDestructible<Key>) {
                        std::destroy_at(key_data() + i);
                    }
                    if constexpr (!traits::TriviallyDestructible<Value>) {
                        std::destroy_at(value_data() + i);
                    }
                }
            }
        }

        for (auto& m : metadata_) { m = Metadata::make_open_slot(); }
        size_ = 0;
    }

  private:
    static constexpr auto HASH_MASK{Capacity - 1};

  private:
    constexpr auto index_of(const Key& key) const noexcept -> opt::Option<usize> {
        if (size_ == 0) { return opt::none; }

        const auto hashed{Hash{}(key)};
        const auto fingerprint{Metadata::take_fingerprint(hashed)};

        usize limit{Capacity};
        usize probe{hashed & HASH_MASK};

        auto m{metadata_[probe]};
        while (!m.is_open() && limit != 0) {
            if (m.is_used() && m.get_fingerprint() == fingerprint) {
                if (Equal{}(key, *(key_data() + probe))) { return probe; }
            }

            limit -= 1;
            probe = (probe + 1) & HASH_MASK;
            m     = metadata_[probe];
        }
        return opt::none;
    }

    // https://en.cppreference.com/cpp/algorithm/swap
    constexpr auto swap(HashMap& other) noexcept -> void {
        using std::swap;
        for (usize i{0}; i < Capacity; ++i) {
            const bool lhs_used{metadata_[i].is_used()};
            const bool rhs_used{other.metadata_[i].is_used()};

            // Metadata swap should always be done since it governs state
            swap(metadata_[i], other.metadata_[i]);

            // There's 3 state cases that need to be handled differently
            if (lhs_used && rhs_used) {
                swap(*(key_data() + i), *(other.key_data() + i));
                swap(*(value_data() + i), *(other.value_data() + i));
            } else if (lhs_used && !rhs_used) {
                std::construct_at(other.key_data() + i, std::move(*(key_data() + i)));
                std::construct_at(other.value_data() + i, std::move(*(value_data() + i)));

                std::destroy_at(key_data() + i);
                std::destroy_at(value_data() + i);
            } else if (!lhs_used && rhs_used) {
                std::construct_at(key_data() + i, std::move(*(other.key_data() + i)));
                std::construct_at(value_data() + i, std::move(*(other.value_data() + i)));

                std::destroy_at(other.key_data() + i);
                std::destroy_at(other.value_data() + i);
            }
        }
        swap(size_, other.size_);
    }

    // ADL dispatcher for copy assignment
    constexpr friend auto swap(HashMap& lhs, HashMap& rhs) noexcept -> void { lhs.swap(rhs); }

  private:
    std::array<Metadata, Capacity> metadata_{};
    Storage<Key, Capacity>         keys_;
    Storage<Value, Capacity>       values_;
    usize                          size_{0};
};

} // namespace detail

// A fixed-size zero-allocation container supporting hash-map operations
template <
    typename Key,
    typename Value,
    usize Capacity,
    typename Hash =
        std::conditional_t<traits::StringLike<Key>, hash::crc::Hash<Key>, hash::Hash<Key>>,
    typename Compare =
        std::conditional_t<traits::StringLike<Key>, hash::StringLikeEq<Key>, std::equal_to<Key>>>
using HashMap = detail::HashMap<Key, Value, ceil_power_of_two(Capacity), Hash, Compare>;

// Construct a hash map from a list of pairs
template <traits::InsertablePair... Pairs>
    requires(sizeof...(Pairs) > 0)
[[nodiscard]] constexpr auto make_hash_map(Pairs&&... kv_pairs) noexcept {
    using std::get;
    HashMap<traits::common_tuple_type_t<0, Pairs...>,
            traits::common_tuple_type_t<1, Pairs...>,
            sizeof...(Pairs)>
        map;
    (...,
     map.emplace(get<0>(std::forward<decltype(kv_pairs)>(kv_pairs)),
                 get<1>(std::forward<decltype(kv_pairs)>(kv_pairs))));
    return map;
}

} // namespace ghoti::fixed
