#include "sparse_triangular_matrix.h"

namespace gspan {

void SparseTriangularMatrix::IncrementCount(int i, int j) {
    auto [row, col] = std::minmax(i, j);
    matrix_[row][col]++;
}

int SparseTriangularMatrix::GetSupport(int i, int j) const {
    auto [row, col] = std::minmax(i, j);

    auto it_row = matrix_.find(row);
    if (it_row == matrix_.end()) return 0;

    auto it_col = it_row->second.find(col);
    if (it_col == it_row->second.end()) return 0;

    return it_col->second;
}

void SparseTriangularMatrix::SetSupport(int i, int j, int sup) {
    auto [row, col] = std::minmax(i, j);
    matrix_[row][col] = sup;
}

void SparseTriangularMatrix::RemoveInfrequent(int minsup) {
    for (auto it_row = matrix_.begin(); it_row != matrix_.end();) {
        auto& inner_map = it_row->second;

        for (auto it_col = inner_map.begin(); it_col != inner_map.end();) {
            if (it_col->second < minsup) {
                it_col = inner_map.erase(it_col);
            } else {
                it_col++;
            }
        }

        if (inner_map.empty()) {
            it_row = matrix_.erase(it_row);
        } else {
            it_row++;
        }
    }
}

}  // namespace gspan