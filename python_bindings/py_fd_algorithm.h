#pragma once

#include <vector>

#include "py_fd.h"
#include "py_primitive.h"

namespace python_bindings {

template <typename PrimitiveType>
class PyFDAlgorithm : public PyPrimitive<PrimitiveType> {
private:
    using Base = PyPrimitive<PrimitiveType>;
    using Base::primitive_;

public:
    std::vector<PyFD> GetResults() {
        std::vector<PyFD> fd_vec;
        for (FD const& fd : primitive_.FdList()) {
            fd_vec.emplace_back(fd.ToRawFD());
        }
        return fd_vec;
    }
};

}  // namespace python_bindings
