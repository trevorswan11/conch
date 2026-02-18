#pragma once

#include <memory>
#include <utility>

namespace conch {

template <typename T> using Box = std::unique_ptr<T>;
template <typename T, typename... Args> constexpr auto make_box(Args&&... args) -> Box<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Makes a new box from an existing pointer
template <typename T, typename P> constexpr auto box_from(P* ptr) -> Box<T> {
    return std::unique_ptr<T>(static_cast<T*>(ptr));
}

// Makes a new box from an existing box, changing the type as requested
template <typename T, typename P> constexpr auto box_into(Box<P>&& ptr) -> Box<T> {
    return std::unique_ptr<T>(static_cast<T*>(ptr.release()));
}

template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace conch
