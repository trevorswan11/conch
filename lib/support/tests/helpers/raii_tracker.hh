#pragma once

#include "types.hh"

namespace ghoti::tests::helpers {

// Non-thread-safe tracker for memory-critical testing
struct RAIITracker {
    static inline i32 live_count{0};
    static inline i32 copy_count{0};
    static inline i32 move_count{0};
    static inline i32 destruct_count{0};

    static auto reset() -> void { live_count = copy_count = move_count = destruct_count = 0; }

    // Dummy variable for constructor to prevent default construction
    RAIITracker(i32) { live_count++; }
    ~RAIITracker() {
        live_count--;
        destruct_count++;
    }

    RAIITracker(const RAIITracker&) {
        live_count++;
        copy_count++;
    }

    auto operator=(const RAIITracker&) -> RAIITracker& {
        copy_count++;
        return *this;
    }

    RAIITracker(RAIITracker&&) noexcept {
        live_count++;
        move_count++;
    }

    auto operator=(RAIITracker&&) noexcept -> RAIITracker& {
        live_count++;
        move_count++;
        return *this;
    }
};

} // namespace ghoti::tests::helpers
