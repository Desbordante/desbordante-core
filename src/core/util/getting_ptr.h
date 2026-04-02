#pragma once

#include <memory>

namespace util {
template <typename T>
T const* GetPointer(T const* ptr) {
    return ptr;
}

template <typename T>
T const* GetPointer(std::unique_ptr<T> const& ptr) {
    return ptr.get();
};
}  // namespace util
