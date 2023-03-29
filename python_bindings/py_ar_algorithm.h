#pragma once

#include <list>

#include <pybind11/pybind11.h>

#include "algorithms/ar_algorithm.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyArAlgorithmBase : public PyAlgorithmBase {
public:
    using PyAlgorithmBase::PyAlgorithmBase;

    [[nodiscard]] std::list<model::ARStrings> GetARs() const {
        return GetAlgorithm<algos::ARAlgorithm>(algorithm_).GetArStringsList();
    }
};

template <typename T>
class PyArAlgorithm : public PyAlgorithm<T, PyArAlgorithmBase> {};

}  // namespace python_bindings
