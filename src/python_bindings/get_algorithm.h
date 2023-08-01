#pragma once

#include <memory>

#include "algorithms/algorithm.h"

namespace python_bindings {

template <typename T>
[[nodiscard]] T const& GetAlgorithm(std::unique_ptr<algos::Algorithm> const& ptr) {
    return dynamic_cast<T const&>(*ptr);
}

}  // namespace python_bindings
