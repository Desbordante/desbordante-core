#pragma once

#include <boost/dynamic_bitset.hpp>
#include <utility>
#include <vector>

#include "hyfd/elements/raw_fd.h"

namespace algos::hyfd::fd_tree {

class FDTreeVertex;

/**
 * Pair of pointer ot FD tree node and the corresponding LHS.
 */
using LhsPair = std::pair<std::shared_ptr<FDTreeVertex>, boost::dynamic_bitset<>>;

/**
 * Node of FD prefix tree.
 *
 * LHS of the FD is represented by the path to the node, besides the path must be built in ascending
 * order, i.e. LHS {0, 1} can be obtained by getting child with position 0, then its child with
 * position 1. If we go first to child 1, it will not contain child 0.
 *
 * RHS of the FD is represented by the fds attribute of the node.
 */
class FDTreeVertex : public std::enable_shared_from_this<FDTreeVertex> {
private:
    std::vector<std::shared_ptr<FDTreeVertex>> children_;
    boost::dynamic_bitset<> fds_;

    /**
     * Union of children RHSs
     */
    boost::dynamic_bitset<> attributes_;

    /**
     * Total number of attributes in the relation
     */
    size_t num_attributes_;

    /**
     * Flag for optimizing child existence check. Is true iff any children_ is set
     */
    bool contains_children_ = false;


public:
    explicit FDTreeVertex(size_t numAttributes) noexcept
        : fds_(numAttributes), attributes_(numAttributes), num_attributes_(numAttributes) {}

};

}  // namespace algos::hyfd::fd_tree
