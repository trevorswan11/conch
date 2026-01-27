#pragma once

#include <utility>

template <typename Signature> class Thunk;
template <typename Ret, typename... Args> class Thunk<Ret(Args...)> {
  public:
    Thunk() = default;
    template <typename ActualRet> explicit Thunk(ActualRet (*fn)(Args...)) {
        fn_ptr_ = reinterpret_cast<void*>(fn);

        thunk_ = [](void* ptr, Args&&... args) -> Ret {
            auto* original_fn = reinterpret_cast<ActualRet (*)(Args...)>(ptr);
            return original_fn(std::forward<Args>(args)...);
        };
    }

    Ret      operator()(Args... args) const { return thunk_(fn_ptr_, std::forward<Args>(args)...); }
    explicit operator bool() const { return thunk_ != nullptr; }

  private:
    void* fn_ptr_                   = nullptr;
    Ret (*thunk_)(void*, Args&&...) = nullptr;
};
