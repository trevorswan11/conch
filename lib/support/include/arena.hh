#pragma once

#include <cstddef>
#include <utility>

#include <gsl/pointers>
#include <gsl/span>

#include "assert.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::mem {

// Do not free returned memory directly!
class Arena {
  public:
    static constexpr usize BLOCK_SIZE{64UZ * 1'024UZ};

  public:
    Arena() noexcept = default;
    ~Arena() { clear(); }

    Arena(const Arena&)                    = delete;
    auto operator=(const Arena&) -> Arena& = delete;
    Arena(Arena&& other) noexcept
        : offset_{other.offset_}, head_{other.head_}, current_{other.current_} {
        other.head_ = nullptr;
        other.reset();
    }
    auto operator=(Arena&&) -> Arena& = delete;

    // cppcheck-suppress-begin [unreadVariable, internalAstError]

    // Asserts that the requested type is at most 64KB
    template <traits::TriviallyDestructible T, typename... Args>
        requires(sizeof(T) <= BLOCK_SIZE)
    [[nodiscard]] auto make(Args&&... args) -> gsl::not_null<T*> {
        void* mem = alloc(sizeof(T), alignof(T));
        return new (mem) T{std::forward<Args>(args)...};
    }

    // Asserts that the requested type-count product can fit in 64KB
    template <traits::TriviallyDestructible T>
        requires(sizeof(T) <= BLOCK_SIZE)
    [[nodiscard]] auto make_span(usize count) -> gsl::span<T> {
        const auto size{sizeof(T) * count};
        ASSERT(size <= BLOCK_SIZE, "Block size cannot fit requested type count");
        void* mem = alloc(size, alignof(T));
        return gsl::span{new (mem) T[count]{}, count};
    }
    // cppcheck-suppress-end [unreadVariable, internalAstError]

    // Extremely efficient, does not invalidate any allocations until rewritten
    auto reset() noexcept -> void {
        current_ = head_;
        offset_  = 0;
    }

    // Deallocates all memory associated with the arena.
    auto clear() noexcept -> void;

  private:
    [[nodiscard]] auto alloc(usize size, usize align) -> void*;

  private:
    struct Block {
        Block* next{nullptr};

        // Allocates a new block housed inside of its own memory region based on `BLOCK_SIZE`
        [[nodiscard]] static auto alloc(Arena& a, usize size, usize align) -> void*;
    };

  private:
    usize  offset_{0};
    Block* head_{nullptr};
    Block* current_{nullptr};
};

} // namespace ghoti::mem
