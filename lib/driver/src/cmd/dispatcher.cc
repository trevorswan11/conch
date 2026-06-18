#include "cmd/dispatcher.hh"

#include <stdx/profiler.hh>
#include <stdx/result.hh>
#include <stdx/types.hh>

#include "cmd/debug.hh"

namespace ghoti::cmd {

auto Dispatcher::operator()(Debug& dump) -> stdx::Result<void, i32> {
    PROFILE_FUNCTION();
    dump.run();
    return {};
}

} // namespace ghoti::cmd
