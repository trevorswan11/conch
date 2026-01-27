#pragma once

#include <utility>

template <typename Signature> class Thunk;
template <typename Ret, typename... Args> class Thunk<Ret(Args...)> {
  public:
    template <typename ActualRet>
    explicit Thunk(ActualRet (*fn)(Args...)) : ptr_{reinterpret_cast<void*>(fn)} {
        thunk_ = [](void* ptr, Args&&... args) -> Ret {
            auto* original_fn = reinterpret_cast<ActualRet (*)(Args...)>(ptr);
            return original_fn(std::forward<Args>(args)...);
        };
    }

    Ret operator()(Args... args) const { return thunk_(ptr_, std::forward<Args>(args)...); }

  private:
    void* ptr_                      = nullptr;
    Ret (*thunk_)(void*, Args&&...) = nullptr;
};
