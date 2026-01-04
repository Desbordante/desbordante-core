#include "core/algorithms/ind/faida/util/simple_ind.h"

#include <cassert>

namespace algos::faida {

bool SimpleIND::operator<(SimpleIND const& other) const {
    if (this->left_->GetTableIndex() != other.left_->GetTableIndex()) {
        return this->left_->GetTableIndex() < other.left_->GetTableIndex();
    } else if (this->right_->GetTableIndex() != other.right_->GetTableIndex()) {
        return this->right_->GetTableIndex() < other.right_->GetTableIndex();
    }

    assert(this->GetArity() == other.GetArity());
    model::ArityIndex const prefix_length = this->GetArity() - 1;
    std::vector<ColumnIndex> const& this_left_columns = this->left_->GetColumnIndices();
    std::vector<ColumnIndex> const& this_right_columns = this->right_->GetColumnIndices();
    std::vector<ColumnIndex> const& other_left_columns = other.left_->GetColumnIndices();
    std::vector<ColumnIndex> const& other_right_columns = other.right_->GetColumnIndices();

    for (model::ArityIndex idx = 0; idx < prefix_length; idx++) {
        if (this_left_columns[idx] != other_left_columns[idx]) {
            return this_left_columns[idx] < other_left_columns[idx];
        }
        if (this_right_columns[idx] != other_right_columns[idx]) {
            return this_right_columns[idx] < other_right_columns[idx];
        }
    }

    if (this->left_->GetLastColumn() != other.left_->GetLastColumn()) {
        return this->left_->GetLastColumn() < other.left_->GetLastColumn();
    }

    return this->right_->GetLastColumn() < other.right_->GetLastColumn();
}

}  // namespace algos::faida
