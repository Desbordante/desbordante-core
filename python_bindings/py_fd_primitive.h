#pragma once

#include <sstream>
#include <utility>
#include <vector>

#include <pybind11/stl.h>

#include "model/fd.h"
#include "model/raw_fd.h"
#include "py_primitive.h"

namespace python_bindings {

class PyFD {
    std::vector<unsigned int> lhs_indices_{};
    size_t rhs_index_;

public:
    explicit PyFD(RawFD const& fd) : rhs_index_(fd.rhs_) {
        for (size_t index = fd.lhs_.find_first(); index != boost::dynamic_bitset<>::npos;
             index = fd.lhs_.find_next(index)) {
            lhs_indices_.emplace_back(index);
        }
    }

    [[nodiscard]] std::string ToString() const {
        std::stringstream stream;
        stream << "( ";
        for (auto index : lhs_indices_) {
            stream << index << " ";
        }
        stream << ") -> ";
        stream << rhs_index_;
        return stream.str();
    }

    [[nodiscard]] pybind11::int_ GetRhs() const {
        return rhs_index_;
    }

    [[nodiscard]] std::vector<unsigned int> GetLhs() const {
        return lhs_indices_;
    }
};

template <typename PrimitiveType>
class PyFDPrimitive : public PyPrimitive<PrimitiveType> {
private:
    using Base = PyPrimitive<PrimitiveType>;
    using Base::primitive_;

public:
    std::vector<PyFD> GetResults() {
        std::vector<PyFD> fd_vec;
        for (FD const& fd : primitive_.FdList()) {
            fd_vec.emplace_back(PyFD(fd.ToRawFD()));
        }
        return fd_vec;
    }
};

}  // namespace python_bindings
