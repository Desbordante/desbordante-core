#pragma once

#include <vector>

#include "algorithms/fd_algorithm.h"
#include "get_algorithm.h"
#include "model/fd.h"
#include "py_algorithm.h"
#include "py_fd.h"

namespace python_bindings {

class PyFdAlgorithmBase : public PyAlgorithmBase {
public:
    using PyAlgorithmBase::PyAlgorithmBase;

    [[nodiscard]] std::vector<PyFD> GetFDs() const {
        std::vector<PyFD> fd_vec;
        for (FD const& fd : GetAlgorithm<algos::FDAlgorithm>(algorithm_).FdList()) {
            fd_vec.emplace_back(fd.ToRawFD());
        }
        return fd_vec;
    }
};

template <typename T>
using PyFDAlgorithm = PyAlgorithm<T, PyFdAlgorithmBase>;

}  // namespace python_bindings
