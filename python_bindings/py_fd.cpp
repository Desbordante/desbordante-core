#include "py_fd.h"

#include <sstream>

namespace python_bindings {

PyFD::PyFD(RawFD const& fd) : rhs_index_(fd.rhs_) {
    for (size_t index = fd.lhs_.find_first(); index != boost::dynamic_bitset<>::npos;
         index = fd.lhs_.find_next(index)) {
        lhs_indices_.emplace_back(index);
    }
}

std::string PyFD::ToString() const {
    std::stringstream stream;
    stream << "( ";
    for (util::config::IndexType index : lhs_indices_) {
        stream << index << " ";
    }
    stream << ") -> ";
    stream << rhs_index_;
    return stream.str();
}

}  // namespace python_bindings
