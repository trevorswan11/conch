#pragma once

#include <stdx/result.hh>
#include <stdx/types.hh>
#include <stdx/variant.hh>

namespace ghoti::cmd {

class Debug;

class Dispatcher {
  public:
    static auto operator()(Debug& dump) -> stdx::Result<void, i32>;
    static auto operator()(stdx::Unit) noexcept -> stdx::Result<void, i32> { return {}; }
};

} // namespace ghoti::cmd
