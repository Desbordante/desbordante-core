#include "py_ucc.h"

#include <sstream>

#include "util/bitset_utils.h"

namespace python_bindings {

PyUCC::PyUCC(model::RawUCC const& raw_ucc)
    : ucc_indices_(util::BitsetToIndices<config::IndexType>(raw_ucc)) {}

std::string PyUCC::ToString() const {
    std::stringstream stream;
    stream << "( ";
    for (config::IndexType index : ucc_indices_) {
        stream << index << " ";
    }
    stream << ")";
    return stream.str();
}

}  // namespace python_bindings
