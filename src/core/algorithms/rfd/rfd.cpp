#include "core/algorithms/rfd/rfd.h"

#include <stdexcept>
#include <string>

#include "core/model/table/idataset_stream.h"

namespace algos::rfd {

std::string RFD::ToString() const {
    if (!input_table) throw std::runtime_error("Invalid RFD (No table reference)");

    std::string result = "[";
    for (std::size_t i = 0; i < lhs_indices.size(); ++i) {
        if (i != 0) result += ", ";
        result += input_table->GetColumnName(lhs_indices[i]);
    }
    result += "] -> [" + input_table->GetColumnName(rhs_index) +
              "] (conf=" + std::to_string(confidence) + ", supp=" + std::to_string(support) + ")";
    return result;
}

}  // namespace algos::rfd
