#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/fd/raw_fd.h"
#include "dynamic_relation_data.h"

namespace algos::dynfd {

/**
 * Pair of pointer of FD tree node and the corresponding LHS.
 */

struct NonFDTreeVertex;

using LhsPair = std::pair<std::shared_ptr<NonFDTreeVertex>, boost::dynamic_bitset<>>;
using ViolatingRecordPair = std::optional<std::pair<size_t, size_t>>;

/**
 * Node of FD prefix tree.
 *
 * LHS of the FD is represented by the path to the node, besides the path must be built in ascending
 * order, i.e. LHS {0, 1} can be obtained by getting child with position 0, then its child with
 * position 1. If we go first to child 1, it will not contain child 0.
 *
 * RHS of the FD is represented by the fds attribute of the node.
 */

struct NonFDTreeVertex : public std::enable_shared_from_this<NonFDTreeVertex> {
private:
    std::vector<std::shared_ptr<NonFDTreeVertex>> children_;
    boost::dynamic_bitset<> non_fds_;
    std::vector<ViolatingRecordPair> violations_;

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
    size_t children_count_ = 0;

    friend class NonFDTree;

    NonFDTreeVertex* GetChild(size_t pos) {
        return children_.at(pos).get();
    }

    boost::dynamic_bitset<> const& GetAttributes() const noexcept {
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
    bool AddChild(size_t pos);

    void GetLevelRecursive(size_t target_level, size_t cur_level, boost::dynamic_bitset<> lhs,
                           std::vector<LhsPair>& vertices);

    void GetNonFdAndSpecialsRecursive(boost::dynamic_bitset<> const& lhs,
                                      boost::dynamic_bitset<>& cur_lhs, size_t rhs, size_t cur_bit,
                                      std::vector<boost::dynamic_bitset<>>& result) const;

    bool ContainsNonFdOrSpecialRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                         size_t cur_bit) const;

    bool RemoveRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs, size_t current_lhs_attr);

    void RemoveGeneralsRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs, size_t cur_bit,
                                 bool is_generalized);

    bool IsLastNodeOf(size_t rhs) const noexcept;

    void FillNonFDs(std::vector<RawFD>& fds, boost::dynamic_bitset<>& lhs) const;

public:
    explicit NonFDTreeVertex(size_t numAttributes) noexcept
        : non_fds_(numAttributes),
          violations_(numAttributes),
          attributes_(numAttributes),
          num_attributes_(numAttributes) {}

    size_t GetNumAttributes() const noexcept {
        return num_attributes_;
    }

    boost::dynamic_bitset<> const& GetNonFDs() const noexcept {
        return non_fds_;
    }

    void SetViolation(size_t pos, ViolatingRecordPair violationPair) {
        violations_[pos] = std::move(violationPair);
    }

    void SetNonFd(size_t pos, ViolatingRecordPair violationPair) {
        non_fds_.set(pos);
        SetViolation(pos, std::move(violationPair));
    }

    /**
     * Replaces stored RHS with provided one.
     * @param new_non_fds RHS to replace with.
     * @param violations
     * */
    void SetNonFds(boost::dynamic_bitset<> new_non_fds,
                   std::vector<ViolatingRecordPair> violations) noexcept {
        non_fds_ = std::move(new_non_fds);
        violations_ = std::move(violations);
    }

    void RemoveNonFd(size_t const pos) noexcept {
        non_fds_.reset(pos);
        violations_[pos] = std::nullopt;
    }

    bool IsNonFd(size_t const pos) const noexcept {
        return non_fds_.test(pos);
    }

    bool IsNonFdViolatingPairHolds(size_t const pos,
                                   std::shared_ptr<DynamicRelationData> relation_);

    std::shared_ptr<NonFDTreeVertex> GetChildPtr(size_t const pos) {
        return children_.at(pos);
    }

    NonFDTreeVertex const* GetChild(size_t const pos) const {
        return children_.at(pos).get();
    }

    std::shared_ptr<NonFDTreeVertex> GetChildIfExists(size_t pos) const;

    bool ContainsChildAt(size_t const pos) const {
        return !children_.empty() && children_.at(pos) != nullptr;
    }

    bool HasChildren() const noexcept {
        return children_count_ > 0;
    }
};

}  // namespace algos::dynfd
