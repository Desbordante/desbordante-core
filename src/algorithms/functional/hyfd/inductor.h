#pragma once

#include <boost/dynamic_bitset.hpp>

#include "structures/fd_tree.h"
#include "structures/non_fd_list.h"

namespace algos::hyfd {

class Inductor {
private:
    std::shared_ptr<fd_tree::FDTree> tree_;

    void SpecializeTreeForNonFd(boost::dynamic_bitset<> const& lhs_bits, size_t rhs_id);

public:
    explicit Inductor(std::shared_ptr<fd_tree::FDTree> tree) noexcept : tree_(std::move(tree)) {}

    void UpdateFdTree(NonFDList&& non_fds);
};

}  // namespace algos::hyfd
