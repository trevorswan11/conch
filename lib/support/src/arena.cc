#include "arena.hh"

#include "assert.hh"
#include "profiler.hh"
#include "types.hh"

namespace ghoti::mem {

// https://github.com/trevorswan11/ghoti/blob/772707146faa9315c24fb079fd759f3715442db1/old/src/util/arena.c
auto Arena::alloc(usize size, usize align) -> void* {
    PROFILE_FUNCTION();
    if (current_) {
        auto        raw_addr{reinterpret_cast<uptr>(current_ + 1)};
        uptr        current_ptr{raw_addr + offset_};
        uptr        aligned_ptr{(current_ptr + (align - 1)) & ~(align - 1)};
        const usize total_size{aligned_ptr - raw_addr + size};

        if (total_size <= BLOCK_SIZE) {
            offset_ = total_size;
            return reinterpret_cast<void*>(aligned_ptr);
        }

        if (current_->next) {
            current_ = current_->next;
            offset_  = 0;
            return alloc(size, align);
        }
    }

    // Otherwise a new block needs to be created for the memory
    return Block::alloc(*this, size, align);
}

auto Arena::clear() noexcept -> void {
    PROFILE_FUNCTION();
    Block* block = head_;
    while (block) {
        Block* next = block->next;
        ::operator delete(block);
        block = next;
    }
    reset();
}

auto Arena::Block::alloc(Arena& a, usize size, usize align) -> void* {
    PROFILE_FUNCTION();
    void* raw   = ::operator new(sizeof(Block) + BLOCK_SIZE);
    auto* block = new (raw) Block{};

    if (!a.head_) {
        a.head_ = block;
    } else {
        ASSERT(a.current_);
        a.current_->next = block;
    }

    a.current_ = block;
    a.offset_  = 0;
    return a.alloc(size, align);
}

} // namespace ghoti::mem
