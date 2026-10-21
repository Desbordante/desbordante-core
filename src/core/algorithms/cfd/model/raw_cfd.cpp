#include "core/algorithms/cfd/model/raw_cfd.h"

#include <sstream>

namespace algos::cfd {

std::string RawCFD::RawItem::ToString() const {
    std::stringstream ss;
    ss << "(" << attribute;
    ss << ", ";
    if (value.has_value()) {
        ss << value.value();
    } else {
        ss << "_";
    }
    ss << ")";
    return ss.str();
}

std::string RawCFD::ToString() const {
    std::stringstream ss;
    ss << "{";
    for (auto it = lhs_.begin(); it != lhs_.end(); ++it) {
        if (it != lhs_.begin()) {
            ss << ",";
        }
        ss << it->ToString();
    }
    ss << "} -> " << rhs_.ToString();
    return ss.str();
}

}  // namespace algos::cfd
