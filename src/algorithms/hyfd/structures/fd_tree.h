#pragma once

#include <boost/dynamic_bitset.hpp>
#include <vector>

#include "fd_tree_vertex.h"
#include "hyfd/elements/raw_fd.h"

namespace algos::hyfd::fd_tree {

/**
 * FD prefix tree.
 *
 * Provides global tree manipulation and traversing methods.
 *
 * @see FDTreeVertex
 */
class FDTree {
private:
    std::shared_ptr<FDTreeVertex> root_;

public:
    explicit FDTree(size_t num_attributes) : root_(std::make_shared<FDTreeVertex>(num_attributes)) {}

    /**
     * @return vector of all FDs
     */
    [[nodiscard]] std::vector<RawFD> FillFDs() const;
};

}  // namespace algos::hyfd::fd_tree
