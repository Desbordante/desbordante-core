#include "core/algorithms/rfd/rfd.h"

#include <string>

namespace algos::rfd {

std::string RFD::ToString() const {
    std::string result = "[";
    bool first = true;
    for (uint8_t attribute = 0; attribute <= kMaxAttributes; ++attribute) {
        if ((lhs_mask & (1u << attribute)) == 0) continue;
        if (!first) result += ", ";

        result += std::to_string(attribute);
        first = false;
    }
    result += "] -> " + std::to_string(rhs_index) + " (conf=" + std::to_string(confidence) +
              ", supp=" + std::to_string(support) + ")";
    return result;
}

}  // namespace algos::rfd
