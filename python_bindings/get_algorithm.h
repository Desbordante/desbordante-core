#pragma once

#include <memory>

#include "algorithms/primitive.h"

namespace python_bindings {

template <typename T>
[[nodiscard]] T const& GetAlgorithm(std::unique_ptr<algos::Primitive> const& ptr) {
    return dynamic_cast<T const&>(*ptr);
}

}  // namespace python_bindings
