#pragma once

#include <cstddef>
#include <ranges>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace algos::hymd::preprocessing::ccv_id_pickers {
template <typename T>
class IndexUniform final {
    std::size_t size_limit_;

public:
    IndexUniform(std::size_t size_limit) : size_limit_(size_limit) {}

    std::vector<ColumnClassifierValueId> operator()(std::vector<T> const& ccvs) const {
        std::size_t ccv_number = ccvs.size();
        std::vector<ColumnClassifierValueId> lhs_ccv_ids;
        if (size_limit_ == 0 || ccv_number <= size_limit_) {
            lhs_ccv_ids.reserve(ccv_number);
            auto iota = std::views::iota(0ul, ccv_number);
            lhs_ccv_ids.assign(iota.begin(), iota.end());
        } else {
            lhs_ccv_ids.reserve(size_limit_ + 1);
            lhs_ccv_ids.push_back(kLowestCCValueId);
            --ccv_number;
            std::size_t const add_index = ccv_number / size_limit_;
            std::size_t const add_rem = ccv_number % size_limit_;
            std::size_t rem = add_rem;
            std::size_t index = add_index;
            while (index <= ccv_number) {
                lhs_ccv_ids.push_back(index);
                index += add_index;
                rem += add_rem;
                if (rem >= size_limit_) {
                    ++index;
                    rem -= size_limit_;
                }
            }
        }
        return lhs_ccv_ids;
    }
};
}  // namespace algos::hymd::preprocessing::ccv_id_pickers
