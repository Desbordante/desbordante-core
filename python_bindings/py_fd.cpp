#include "py_fd.h"

#include <sstream>

#include "util/bitset_utils.h"

namespace python_bindings {

PyFD::PyFD(RawFD const& fd)
    : lhs_indices_(util::BitsetToIndices<decltype(lhs_indices_)::value_type>(fd.lhs_)),
      rhs_index_(fd.rhs_) {}

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
