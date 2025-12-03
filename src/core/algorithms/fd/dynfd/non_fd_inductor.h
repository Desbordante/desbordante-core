#pragma once

#include <vector>

#include "model/FDTrees/fd_tree.h"
#include "model/non_fd_tree.h"
#include "validator.h"

namespace algos::dynfd {
class NonFDInductor {
    std::shared_ptr<model::FDTree> positive_cover_tree_;
    std::shared_ptr<NonFDTree> negative_cover_tree_;
    std::shared_ptr<Validator> validator_;

    void Dfs(RawFD fd, size_t next_lhs_attr);

    void DeduceNonFds(boost::dynamic_bitset<> const& lhs, size_t rhs);

public:
    NonFDInductor(std::shared_ptr<model::FDTree> positive_cover_tree,
                  std::shared_ptr<NonFDTree> negative_cover_tree,
                  std::shared_ptr<Validator> validator) noexcept
        : positive_cover_tree_(std::move(positive_cover_tree)),
          negative_cover_tree_(std::move(negative_cover_tree)),
          validator_(std::move(validator)) {}

    void FindFds(std::vector<RawFD> const& valid_fds);
};
}  // namespace algos::dynfd
