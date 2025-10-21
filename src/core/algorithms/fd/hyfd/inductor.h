#pragma once

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hyfd/model/non_fd_list.h"
#include "core/model/FDTrees/fd_tree.h"

namespace algos::hyfd {

class Inductor {
private:
    std::shared_ptr<model::FDTree> tree_;

    void SpecializeTreeForNonFd(boost::dynamic_bitset<> const& lhs_bits, size_t rhs_id);

public:
    explicit Inductor(std::shared_ptr<model::FDTree> tree) noexcept : tree_(std::move(tree)) {}

    void UpdateFdTree(NonFDList const& non_fds);
};

}  // namespace algos::hyfd
