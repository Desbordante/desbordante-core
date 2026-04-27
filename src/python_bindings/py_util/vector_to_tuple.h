#pragma once

#include <pybind11/pybind11.h>

#include <functional>
#include <vector>

namespace python_bindings {
template <typename ElementType, typename Func>
pybind11::tuple VectorToTuple(std::vector<ElementType> const& vec, Func&& func) {
    pybind11::tuple tup(vec.size());
    for (std::size_t i = 0; i < vec.size(); ++i) {
        tup[i] = std::invoke(func, vec[i]);
    }
    return tup;
}

template <typename ElementType>
pybind11::tuple VectorToTuple(std::vector<ElementType> const& vec) {
    pybind11::tuple tup(vec.size());
    for (std::size_t i = 0; i < vec.size(); i++) {
        tup[i] = vec[i];
    }
    return tup;
}

}  // namespace python_bindings
