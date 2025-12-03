#pragma once

#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/fd/raw_fd.h"
#include "non_fd_tree_vertex.h"

namespace algos::dynfd {

/**
 * NonFD prefix tree.
 *
 * Provides global tree manipulation and traversing methods.
 *
 * @see NonFDTreeVertex
 */

class NonFDTree {
private:
    std::shared_ptr<NonFDTreeVertex> root_;

public:
    explicit NonFDTree(size_t num_attributes)
        : root_(std::make_shared<NonFDTreeVertex>(num_attributes)) {}

    [[nodiscard]] size_t GetNumAttributes() const noexcept {
        return root_->GetNumAttributes();
    }

    std::shared_ptr<NonFDTreeVertex> GetRootPtr() noexcept {
        return root_;
    }

    [[nodiscard]] NonFDTreeVertex const& GetRoot() const noexcept {
        return *root_;
    }

    void AddNonFD(boost::dynamic_bitset<> const& lhs, size_t rhs,
                  ViolatingRecordPair violationPair);

    bool ContainsNonFD(boost::dynamic_bitset<> const& lhs, size_t rhs);

    std::shared_ptr<NonFDTreeVertex> FindNonFdVertex(boost::dynamic_bitset<> const& lhs);

    /**
     * Recursively finds node representing given lhs and removes given rhs bit from it.
     * Destroys vertices whose children became empty.
     */
    void Remove(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        root_->RemoveRecursive(lhs, rhs, lhs.find_first());
    }

    void RemoveGenerals(boost::dynamic_bitset<> const& lhs, size_t rhs);

    std::vector<boost::dynamic_bitset<>> GetNonFdAndSpecials(boost::dynamic_bitset<> const& lhs,
                                                             size_t rhs);

    [[nodiscard]] bool ContainsNonFdOrSpecial(boost::dynamic_bitset<> const& lhs, size_t rhs) const;

    /**
     * Gets nodes representing NonFDs with LHS of given arity.
     * @param target_level arity of returned NonFDs LHSs
     */
    std::vector<LhsPair> GetLevel(size_t target_level);

    /**
     * @return vector of all NonFDs
     */
    [[nodiscard]] std::vector<RawFD> FillNonFDs() const;
};
}  // namespace algos::dynfd
