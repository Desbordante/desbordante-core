#include "simple_ind.h"

#include "cassert"

namespace algos::faida {

bool SimpleIND::operator<(SimpleIND const& other) const {
    if (this->left_->GetTableIndex() != other.left_->GetTableIndex()) {
        return this->left_->GetTableIndex() < other.left_->GetTableIndex();
    } else if (this->right_->GetTableIndex() != other.right_->GetTableIndex()) {
        return this->right_->GetTableIndex() < other.right_->GetTableIndex();
    }

    assert(this->GetArity() == other.GetArity());
    size_t const prefix_length = this->GetArity() - 1;
    std::vector<ColumnIndex> const& this_left_columns = this->left_->GetColumnIndices();
    std::vector<ColumnIndex> const& this_right_columns = this->right_->GetColumnIndices();
    std::vector<ColumnIndex> const& other_left_columns = other.left_->GetColumnIndices();
    std::vector<ColumnIndex> const& other_right_columns = other.right_->GetColumnIndices();

    for (ColumnIndex col_idx = 0; col_idx < prefix_length; col_idx++) {
        if (this_left_columns[col_idx] != other_left_columns[col_idx]) {
            return this_left_columns[col_idx] < other_left_columns[col_idx];
        }
        if (this_right_columns[col_idx] != other_right_columns[col_idx]) {
            return this_right_columns[col_idx] < other_right_columns[col_idx];
        }
    }

    if (this->left_->GetLastColumn() != other.left_->GetLastColumn()) {
        return this->left_->GetLastColumn() < other.left_->GetLastColumn();
    }

    return this->right_->GetLastColumn() < other.right_->GetLastColumn();
}

}  // namespace algos::faida
