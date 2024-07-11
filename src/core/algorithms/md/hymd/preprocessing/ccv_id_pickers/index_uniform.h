#pragma once

#include <cstddef>
#include <ranges>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace algos::hymd::preprocessing::ccv_id_pickers {
inline std::vector<ColumnClassifierValueId> IndexUniform(std::size_t ccv_number,
                                                         std::size_t const size_limit) {
    std::vector<ColumnClassifierValueId> lhs_ids;
    if (size_limit == 0 || ccv_number <= size_limit) {
        lhs_ids.reserve(ccv_number);
        auto iota = std::views::iota(0ul, ccv_number);
        lhs_ids.assign(iota.begin(), iota.end());
    } else {
        lhs_ids.reserve(size_limit + 1);
        lhs_ids.push_back(kLowestCCValueId);
        --ccv_number;
        std::size_t const add_index = ccv_number / size_limit;
        std::size_t const add_rem = ccv_number % size_limit;
        std::size_t rem = add_rem;
        std::size_t index = add_index;
        while (index <= ccv_number) {
            lhs_ids.push_back(index);
            index += add_index;
            rem += add_rem;
            if (rem >= size_limit) {
                ++index;
                rem -= size_limit;
            }
        }
    }
    return lhs_ids;
}
}  // namespace algos::hymd::preprocessing::ccv_id_pickers
