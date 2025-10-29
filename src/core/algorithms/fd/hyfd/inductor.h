#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/fd/hyfd/model/fd_tree.h"
#include "algorithms/fd/hyfd/model/non_fd_list.h"

namespace algos {
namespace hyfd {
namespace fd_tree {
class FDTree;
}  // namespace fd_tree
}  // namespace hyfd
}  // namespace algos

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
