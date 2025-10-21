#pragma once

#include "algorithms/fd/hycommon/column_combination_list.h"
#include "model/FDTrees/fd_tree.h"
#include "model/non_fd_tree.h"

namespace algos::dynfd {
class FDInductor {
    std::shared_ptr<model::FDTree> positive_cover_tree_;
    std::shared_ptr<NonFDTree> negative_cover_tree_;

    void DeduceDependencies(boost::dynamic_bitset<> const& lhs, size_t rhs);

public:
    FDInductor(std::shared_ptr<model::FDTree> positive_cover_tree,
               std::shared_ptr<NonFDTree> negative_cover_tree) noexcept
        : positive_cover_tree_(std::move(positive_cover_tree)),
          negative_cover_tree_(std::move(negative_cover_tree)) {}

    void UpdateCovers(algos::hy::ColumnCombinationList const& agree_sets);
};
}  // namespace algos::dynfd
