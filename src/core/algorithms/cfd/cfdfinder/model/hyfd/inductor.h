#pragma once

#include <list>

#include "algorithms/cfd/cfdfinder/candidate.h"
#include "algorithms/fd/hyfd/model/fd_tree.h"
#include "algorithms/fd/hyfd/model/non_fd_list.h"
#include "types/bitset.h"

namespace algos::cfdfinder {

using NonFDList = hyfd::NonFDList;
using FDTree = algos::hyfd::fd_tree::FDTree;

class Inductor {
private:
    std::shared_ptr<FDTree> tree_;
    std::list<Candidate> max_non_fds_;

    void SpecializeTreeForNonFd(BitSet const& lhs_bits, size_t rhs_id);

public:
    explicit Inductor(std::shared_ptr<FDTree> tree) noexcept : tree_(std::move(tree)) {}

    void UpdateFdTree(NonFDList&& non_fds);

    std::list<Candidate> FillMaxNonFDs() {
        return std::move(max_non_fds_);
    }
};

}  // namespace algos::cfdfinder
