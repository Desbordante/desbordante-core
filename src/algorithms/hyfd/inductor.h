#pragma once

#include "structures/fd_tree.h"
#include "structures/non_fd_list.h"

namespace algos::hyfd {

class Inductor {
private:
    std::shared_ptr<fd_tree::FDTree> tree_;

public:
    explicit Inductor(std::shared_ptr<fd_tree::FDTree> tree) noexcept : tree_(std::move(tree)) {}

    void UpdateFdTree(NonFDList non_fds);
};

}  // namespace algos::hyfd
