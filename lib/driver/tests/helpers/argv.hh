#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <stdx/types.hh>

namespace ghoti::tests::helpers {

class MockArgv {
  public:
    explicit MockArgv(std::initializer_list<std::string> args) : strings_{args} {
        REQUIRE(args.size() >= 1);
        pointers_.reserve(strings_.size() + 1);
        for (auto& s : strings_) { pointers_.push_back(s.data()); }
        pointers_.push_back(nullptr);
    }

    [[nodiscard]] auto argc() const noexcept -> i32 { return static_cast<i32>(strings_.size()); }
    [[nodiscard]] auto argv() noexcept -> char** { return pointers_.data(); }

  private:
    std::vector<std::string> strings_;
    std::vector<char*>       pointers_;
};

} // namespace ghoti::tests::helpers
