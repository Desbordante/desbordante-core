#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "raw_fd.h"

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

    friend class FDTree;

    FDTreeVertex* GetChild(size_t pos) {
        return children_.at(pos).get();
    }

    void SetFd(size_t pos) {
        fds_.set(pos);
    }

    boost::dynamic_bitset<> GetAttributes() const noexcept {
        return attributes_;
    }

    void SetAttribute(size_t pos) noexcept {
        attributes_.set(pos);
    }

    void RemoveAttribute(size_t pos) noexcept {
        attributes_.reset(pos);
    }

    bool IsAttribute(size_t pos) const noexcept {
        return attributes_.test(pos);
    }

    /**
     * Constructs empty child node at the given position. Does nothing if the child already exists.
     *
     * @param pos child position
     * @return whether a child was constructed
     */
    bool AddChild(size_t pos) {
        contains_children_ = true;
        if (children_.empty()) {
            children_.resize(num_attributes_);
        }

        if (!ContainsChildAt(pos)) {
            children_[pos] = std::make_shared<FDTreeVertex>(num_attributes_);
            return true;
        }

        return false;
    }

    void GetLevelRecursive(unsigned target_level, unsigned cur_level, boost::dynamic_bitset<> lhs,
                           std::vector<LhsPair>& vertices);

    void GetFdAndGeneralsRecursive(boost::dynamic_bitset<> const& lhs,
                                   boost::dynamic_bitset<> cur_lhs, size_t rhs, size_t cur_bit,
                                   std::vector<boost::dynamic_bitset<>>& result) const;

    bool FindFdOrGeneralRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                  size_t cur_bit) const;

    bool RemoveRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs, size_t current_lhs_attr);

    bool IsLastNodeOf(size_t rhs) const noexcept;

    void FillFDs(std::vector<RawFD>& fds, boost::dynamic_bitset<>& lhs) const;

public:
    explicit FDTreeVertex(size_t numAttributes) noexcept
        : fds_(numAttributes), attributes_(numAttributes), num_attributes_(numAttributes) {}

    size_t GetNumAttributes() const noexcept {
        return num_attributes_;
    }

    boost::dynamic_bitset<> GetFDs() const noexcept {
        return fds_;
    }

    /**
     * Replaces stored RHS with provided one.
     * @param new_fds RHS to replace with.
     * */
    void SetFds(boost::dynamic_bitset<> new_fds) noexcept {
        fds_ = std::move(new_fds);
    }

    void RemoveFd(size_t pos) noexcept {
        fds_.reset(pos);
    }

    bool IsFd(size_t pos) const noexcept {
        return fds_.test(pos);
    }

    std::shared_ptr<FDTreeVertex> GetChildPtr(size_t pos) {
        return children_.at(pos);
    }

    FDTreeVertex const* GetChild(size_t pos) const {
        return children_.at(pos).get();
    }

    bool ContainsChildAt(size_t pos) const {
        return children_.at(pos) != nullptr;
    }

    bool HasChildren() const noexcept {
        return contains_children_;
    }
};

}  // namespace algos::hyfd::fd_tree
