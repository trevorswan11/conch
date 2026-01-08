#include <assert.h>
#include <string.h>

#include "util/arena.h"
#include "util/math.h"
#include "util/memory.h"

static inline void* arena_malloc(void* arena_ctx, size_t size) {
    assert(arena_ctx);
    if (size == 0) { return nullptr; }

    // Space in the arena can be in any block technically
    Arena* arena = arena_ctx;
    while (true) {
        uintptr_t current_addr    = (uintptr_t)&arena->current->data[arena->current->offset];
        uintptr_t aligned_addr    = align_up(current_addr, 8);
        size_t offset_after_alloc = (size_t)(aligned_addr - (uintptr_t)arena->current->data) + size;

        // If we can put it in the current block then its a quick break
        if (offset_after_alloc <= arena->current->capacity) {
            arena->current->offset = offset_after_alloc;
            return (void*)aligned_addr;
        }

        // A previous reset may leave the next block open
        if (arena->current->next != nullptr) {
            arena->current         = arena->current->next;
            arena->current->offset = 0;
            continue;
        }

        // Otherwise a new block is made (scaled to next power of two)
        size_t next_size = ceil_power_of_two_size(arena->previous_block_size + 1);
        if (next_size < size + 8) { next_size = ceil_power_of_two_size(size + 8); }

        ArenaBlock* new_block = ALLOCATOR_MALLOC(arena->child_allocator, sizeof(*new_block));
        uint8_t*    data      = ALLOCATOR_MALLOC(arena->child_allocator, next_size);
        if (!new_block || !data) { return nullptr; }

        *new_block = (ArenaBlock){
            .capacity = next_size,
            .data     = data,
            .next     = nullptr,
            .offset   = 0,
        };

        arena->current->next       = new_block;
        arena->current             = new_block;
        arena->previous_block_size = next_size;
    }
}

static inline void* arena_calloc(void* arena_ctx, size_t count, size_t size) {
    const size_t full_size = count * size;
    void*        mem       = arena_malloc(arena_ctx, full_size);
    if (!mem) { return nullptr; }
    memset(mem, 0, full_size);
    return mem;
}

static inline void* arena_realloc(void* arena_ctx, void* ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) { return nullptr; }
    if (!ptr) { return arena_malloc(arena_ctx, new_size); }
    if (new_size <= old_size) { return ptr; }

    // If the pointer is at the last offset, we can try to grow without copy
    Arena*    arena       = arena_ctx;
    uintptr_t current_top = (uintptr_t)arena->current->data + arena->current->offset;
    if ((uintptr_t)ptr + old_size == current_top) {
        size_t diff = new_size - old_size;
        if (arena->current->offset + diff <= arena->current->capacity) {
            arena->current->offset += diff;
            return ptr;
        }
    }

    // We can allocate and copy since the old size is known
    void* new_ptr = arena_malloc(arena_ctx, new_size);
    if (!new_ptr) { return nullptr; }
    memcpy(new_ptr, ptr, old_size);
    return new_ptr;
}

static inline void arena_free(NO_CTX, [[maybe_unused]] void* ptr) {}

[[nodiscard]] Status
arena_init(Allocator* arena, size_t initial_block_size, Allocator* child_allocator) {
    ASSERT_ALLOCATOR_PTR(child_allocator);
    const size_t true_block = ceil_power_of_two_size(initial_block_size);

    ArenaBlock* initial_block = ALLOCATOR_PTR_MALLOC(child_allocator, sizeof(*initial_block));
    if (!initial_block) { return ALLOCATION_FAILED; }

    uint8_t* block_data = ALLOCATOR_PTR_MALLOC(child_allocator, true_block * sizeof(*block_data));
    if (!block_data) {
        ALLOCATOR_PTR_FREE(child_allocator, initial_block);
        return ALLOCATION_FAILED;
    }

    *initial_block = (ArenaBlock){
        .next     = nullptr,
        .capacity = true_block,
        .offset   = 0,
        .data     = block_data,
    };

    Arena* arena_ctx = ALLOCATOR_PTR_MALLOC(child_allocator, sizeof(*arena_ctx));
    if (!arena_ctx) {
        ALLOCATOR_PTR_FREE(child_allocator, block_data);
        ALLOCATOR_PTR_FREE(child_allocator, initial_block);
        return ALLOCATION_FAILED;
    }

    *arena_ctx = (Arena){
        .child_allocator     = *child_allocator,
        .head                = initial_block,
        .current             = initial_block,
        .previous_block_size = true_block,
    };

    *arena = (Allocator){
        .ctx              = arena_ctx,
        .memory_alloc     = arena_malloc,
        .continuous_alloc = arena_calloc,
        .re_alloc         = arena_realloc,
        .free_alloc       = arena_free,
    };
    return SUCCESS;
}

void arena_deinit(Allocator* arena) {
    if (!arena || !arena->ctx) { return; }

    Arena*     arena_ctx = arena->ctx;
    Allocator* allocator = &arena_ctx->child_allocator;
    ASSERT_ALLOCATOR_PTR(allocator);

    ArenaBlock* block = arena_ctx->head;
    while (block != nullptr) {
        ArenaBlock* next = block->next;
        ALLOCATOR_PTR_FREE(allocator, block->data);
        ALLOCATOR_PTR_FREE(allocator, block);

        block = next;
    }

    ALLOCATOR_PTR_FREE(allocator, arena_ctx);
    arena->ctx = nullptr;
}

void arena_reset(Allocator* arena, ArenaResetMode reset_mode) {
    assert(arena && arena->ctx);

    Arena*      arena_ctx = arena->ctx;
    ArenaBlock* block     = arena_ctx->head;

    switch (reset_mode) {
    case DEFAULT:
    case RETAIN_CAPACITY:
        while (block != nullptr) {
            block->offset = 0;
            block         = block->next;
        }
        break;
    case ZERO_RETAIN_CAPACITY:
        while (block != nullptr) {
            block->offset = 0;
            memset(block->data, 0, block->capacity);
            block = block->next;
        }
        break;
    case FULL_RESET:
        arena_ctx->head->offset = 0;
        block                   = arena_ctx->head->next;
        Allocator allocator     = arena_ctx->child_allocator;

        while (block != nullptr) {
            ArenaBlock* next = block->next;
            ALLOCATOR_FREE(allocator, block->data);
            ALLOCATOR_FREE(allocator, block);

            block = next;
        }

        arena_ctx->head->next = nullptr;
        break;
    }

    arena_ctx->current = arena_ctx->head;
}
