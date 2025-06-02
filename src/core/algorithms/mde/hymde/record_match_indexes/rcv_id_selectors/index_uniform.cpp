#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/index_uniform.h"

#include <ranges>
#include <vector>

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
std::vector<RecordClassifierValueId> IndexUniform::GetSubsetIndices(std::size_t size) const {
    std::vector<RecordClassifierValueId> lhs_rcv_ids;
    if (size_limit_ == 0 || size <= size_limit_) {
        lhs_rcv_ids.reserve(size);
        auto iota = std::views::iota(0ul, size);
        lhs_rcv_ids.assign(iota.begin(), iota.end());
    } else {
        lhs_rcv_ids.reserve(size_limit_ + 1);
        lhs_rcv_ids.push_back(kLowestRCValueId);
        --size;
        std::size_t const add_index = size / size_limit_;
        std::size_t const add_rem = size % size_limit_;
        std::size_t rem = add_rem;
        std::size_t index = add_index;
        while (index <= size) {
            lhs_rcv_ids.push_back(index);
            index += add_index;
            rem += add_rem;
            if (rem >= size_limit_) {
                ++index;
                rem -= size_limit_;
            }
        }
    }
    return lhs_rcv_ids;
}
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
