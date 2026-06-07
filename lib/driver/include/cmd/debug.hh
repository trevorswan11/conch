#pragma once

#include <string>

namespace ghoti::cmd {

class Debug {
  public:
    auto run() -> void;

  private:
    std::string line_;
};

} // namespace ghoti::cmd
