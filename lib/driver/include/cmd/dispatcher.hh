#pragma once

#include <result.hh>
#include <types.hh>
#include <variant.hh>

namespace ghoti::cmd {

class Debug;

class Dispatcher {
  public:
    static auto operator()(Debug& dump) -> Result<void, i32>;
    static auto operator()(Unit) noexcept -> Result<void, i32> { return {}; }
};

} // namespace ghoti::cmd
