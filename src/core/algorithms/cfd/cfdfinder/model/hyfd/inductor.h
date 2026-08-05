#pragma once

#include <cstddef>
#include <list>
#include <memory>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/fd/hycommon/column_combination_list.h"
#include "core/algorithms/fd/hyfd/model/fd_tree.h"

namespace algos::cfdfinder {

using NonFDList = hy::ColumnCombinationList;
using FDTree = algos::hyfd::fd_tree::FDTree;

class Inductor {
private:
    std::shared_ptr<FDTree> tree_;
    std::list<Candidate> max_non_fds_;

    void SpecializeTreeForNonFd(boost::dynamic_bitset<> const& lhs_bits, size_t rhs_id);

public:
    explicit Inductor(std::shared_ptr<FDTree> tree) noexcept : tree_(std::move(tree)) {}

    void UpdateFdTree(NonFDList&& non_fds);

    std::list<Candidate> FillMaxNonFDs() {
        return std::move(max_non_fds_);
    }
};

}  // namespace algos::cfdfinder
