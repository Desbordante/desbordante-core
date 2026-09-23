#include "core/algorithms/rfd/rfd.h"

#include <string>

namespace algos::rfd {

std::string RFD::ToString() const {
    std::string result = "[";
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (i != 0) result += ", ";
        result += lhs[i];
    }
    result += "] -> [" + rhs + "] (conf=" + std::to_string(confidence) +
              ", supp=" + std::to_string(support) + ")";
    return result;
}

}  // namespace algos::rfd
