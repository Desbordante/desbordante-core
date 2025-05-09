#pragma once

#include <memory>    // for shared_ptr
#include <stddef.h>  // for size_t
#include <utility>   // for move

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset

#include "algorithms/fd/hyfd/model/non_fd_list.h"  // for NonFDList

namespace algos {
namespace hyfd {
namespace fd_tree {
class FDTree;
}
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
