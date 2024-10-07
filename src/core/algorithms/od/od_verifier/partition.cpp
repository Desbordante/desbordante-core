#include "partition.h"

#include <utility>
#include <vector>

namespace algos::od_verifier {

std::vector<ComplaxStrippedPartition::ViolationDescription>
ComplaxStrippedPartition::CommonViolationBySplit(model::ColumnIndex right) const {
    std::vector<ComplaxStrippedPartition::ViolationDescription> violates;

    for (size_t begin_pointer = 0; begin_pointer < sp_begins_->size() - 1; begin_pointer++) {
        size_t const group_begin = (*sp_begins_)[begin_pointer];
        size_t const group_end = (*sp_begins_)[begin_pointer + 1];

        int const group_value = data_->GetValue((*sp_indexes_)[group_begin], right);

        for (size_t i = group_begin + 1; i < group_end; i++) {
            if (data_->GetValue((*sp_indexes_)[i], right) != group_value) {
                violates.emplace_back(right, (*sp_indexes_)[i]);
            }
        }
    }

    return violates;
}

std::vector<ComplaxStrippedPartition::ViolationDescription>
ComplaxStrippedPartition::RangeBasedViolationBySplit(model::ColumnIndex right) const {
    std::vector<ComplaxStrippedPartition::ViolationDescription> violates;

    for (size_t begin_pointer = 0; begin_pointer < rb_begins_->size() - 1; ++begin_pointer) {
        size_t const group_begin = (*rb_begins_)[begin_pointer];
        size_t const group_end = (*rb_begins_)[begin_pointer + 1];

        int const group_value = data_->GetValue((*rb_indexes_)[group_begin].first, right);

        for (size_t i = group_begin; i < group_end; ++i) {
            algos::fastod::DataFrame::Range const range = (*rb_indexes_)[i];

            for (size_t j = range.first; j <= range.second; ++j) {
                if (data_->GetValue(j, right) != group_value) {
                    violates.emplace_back(right, j);
                }
            }
        }
    }

    return violates;
}

std::vector<ComplaxStrippedPartition::ViolationDescription>
ComplaxStrippedPartition::FindViolationsBySplit(model::ColumnIndex right) const {
    return is_stripped_partition_ ? CommonViolationBySplit(right)
                                  : RangeBasedViolationBySplit(right);
}

}  // namespace algos::od_verifier
