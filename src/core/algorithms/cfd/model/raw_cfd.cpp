#include "core/algorithms/cfd/model/raw_cfd.h"

#include <sstream>

namespace algos::cfd {

static std::string RawItemToString(RawCFD::RawItem const& item) {
    std::stringstream ss;
    ss << "(" << item.attribute;
    ss << ", ";
    if (item.value.has_value()) {
        ss << item.value.value();
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
        ss << RawItemToString(*it);
    }
    ss << "} -> " << RawItemToString(rhs_);
    return ss.str();
}

}  // namespace algos::cfd
