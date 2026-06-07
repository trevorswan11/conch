#include "cmd/dispatcher.hh"

#include "cmd/debug.hh"

#include "result.hh"
#include "types.hh"

namespace ghoti::cmd {

auto Dispatcher::operator()(Debug& dump) -> Result<void, i32> {
    dump.run();
    return {};
}

} // namespace ghoti::cmd
