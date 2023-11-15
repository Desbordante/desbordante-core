#pragma once

#include <memory>

#include "algorithms/algorithm.h"

namespace python_bindings {

template <typename T>
[[nodiscard]] T& GetAlgorithm(std::unique_ptr<algos::Algorithm> const& ptr) {
    return dynamic_cast<T&>(*ptr);
}

}  // namespace python_bindings
