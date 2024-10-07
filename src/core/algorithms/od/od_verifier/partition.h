#pragma once

#include "algorithms/od/fastod/partitions/complex_stripped_partition.h"

namespace algos::od_verifier {

class ComplaxStrippedPartition : protected algos::fastod::ComplexStrippedPartition {
private:
    using ViolationDescription = std::pair<int, int>;

    std::vector<ViolationDescription> CommonViolationBySplit(model::ColumnIndex right) const;

    std::vector<ViolationDescription> RangeBasedViolationBySplit(model::ColumnIndex right) const;

public:
    ComplaxStrippedPartition() : algos::fastod::ComplexStrippedPartition() {}

    ComplaxStrippedPartition(algos::fastod::ComplexStrippedPartition const& daddy)
        : algos::fastod::ComplexStrippedPartition(daddy) {}

    std::vector<ViolationDescription> FindViolationsBySplit(model::ColumnIndex right) const;

    template <bool Ascending>
    std::vector<ViolationDescription> FindViolationsBySwap(model::ColumnIndex left,
                                                           model::ColumnIndex right) const {
        size_t const group_count = is_stripped_partition_ ? sp_begins_->size() : rb_begins_->size();
        std::vector<ComplaxStrippedPartition::ViolationDescription> violates;

        for (size_t begin_pointer = 0; begin_pointer < group_count - 1; begin_pointer++) {
            size_t const group_begin = is_stripped_partition_ ? (*sp_begins_)[begin_pointer]
                                                              : (*rb_begins_)[begin_pointer];

            size_t const group_end = is_stripped_partition_ ? (*sp_begins_)[begin_pointer + 1]
                                                            : (*rb_begins_)[begin_pointer + 1];

            std::vector<std::pair<int, int>> values;
            std::vector<int> row_pos;

            if (is_stripped_partition_) {
                values.reserve(group_end - group_begin);

                for (size_t i = group_begin; i < group_end; ++i) {
                    size_t const index = (*sp_indexes_)[i];

                    values.emplace_back(data_->GetValue(index, left),
                                        data_->GetValue(index, right));
                    row_pos.emplace_back(index);
                }
            } else {
                for (size_t i = group_begin; i < group_end; ++i) {
                    algos::fastod::DataFrame::Range const range = (*rb_indexes_)[i];

                    for (size_t j = range.first; j <= range.second; ++j) {
                        values.emplace_back(data_->GetValue(j, left), data_->GetValue(j, right));
                    }
                }
            }

            if constexpr (Ascending) {
                std::sort(values.begin(), values.end(),
                          [](auto const& p1, auto const& p2) { return p1.first < p2.first; });
            } else {
                std::sort(values.begin(), values.end(),
                          [](auto const& p1, auto const& p2) { return p2.first < p1.first; });
            }

            size_t prev_group_max_index = 0;
            size_t current_group_max_index = 0;
            bool is_first_group = true;

            for (size_t i = 0; i < values.size(); i++) {
                auto const& [first, second] = values[i];

                if (i != 0 && values[i - 1].first != first) {
                    is_first_group = false;
                    prev_group_max_index = current_group_max_index;
                    current_group_max_index = i;
                } else if (values[current_group_max_index].second <= second) {
                    current_group_max_index = i;
                }

                if (!is_first_group && values[prev_group_max_index].second > second) {
                    violates.emplace_back(right, row_pos[i]);
                }
            }
        }

        return violates;
    }
};

}  // namespace algos::od_verifier
