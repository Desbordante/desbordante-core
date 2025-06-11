#pragma once

#include <vector>

#include "model/FDTrees/fd_tree.h"
#include "model/non_fd_tree.h"
#include "algorithms/fd/hycommon/column_combination_list.h"

namespace algos::dynfd {
class FDInductor {
    std::shared_ptr<model::FDTree> positive_cover_tree_;
    std::shared_ptr<NonFDTree> negative_cover_tree_;

    void DeduceDependencies(RawFD non_fd);

public:
    FDInductor(std::shared_ptr<model::FDTree> positive_cover_tree,
              std::shared_ptr<NonFDTree> negative_cover_tree) noexcept
        : positive_cover_tree_(std::move(positive_cover_tree)),
          negative_cover_tree_(std::move(negative_cover_tree)) {}
    
    void UpdateCovers(algos::hy::ColumnCombinationList const& agree_sets);
};
} // namespace algos::dynfd
