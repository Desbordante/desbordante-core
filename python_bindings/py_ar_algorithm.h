#pragma once

#include <list>

#include <pybind11/pybind11.h>

#include "py_primitive.h"

namespace python_bindings {

template <typename T>
class PyArAlgorithm : public PyPrimitive<T> {
    using Base = PyPrimitive<T>;
    using Base::primitive_;

public:
    std::list<model::ARStrings> GetResults() {
        return primitive_.GetArStringsList();
    }
};

}  // namespace python_bindings
