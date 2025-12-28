#include <assert.h>
#include <string.h>

#include "util/arena.h"
#include "util/math.h"

[[nodiscard]] Status
arena_init(Allocator* arena, size_t initial_block_size, Allocator* child_allocator) {
    ASSERT_ALLOCATOR_PTR(child_allocator);
    const size_t true_block = ceil_power_of_two_size(initial_block_size);

    ArenaBlock* initial_block = ALLOCATOR_PTR_MALLOC(child_allocator, sizeof(ArenaBlock));
    if (!initial_block) { return ALLOCATION_FAILED; }

    uint8_t* block_data = ALLOCATOR_PTR_CALLOC(child_allocator, true_block, sizeof(uint8_t));
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

    Arena* arena_ctx = ALLOCATOR_PTR_MALLOC(child_allocator, sizeof(Arena));
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
}

void arena_reset(Allocator* arena) {
    assert(arena && arena->ctx);

    Arena*      arena_ctx = arena->ctx;
    ArenaBlock* block     = arena_ctx->head;
    while (block != nullptr) {
        block->offset = 0;
        memset(block->data, 0, block->capacity);
        block = block->next;
    }

    arena_ctx->current = arena_ctx->head;
}
