#pragma once

#include <list>
#include <unordered_map>

#include "algorithms/cfd/cfdfinder/util/lhs_utils.h"
#include "raw_cfd.h"

namespace algos::cfdfinder {
class ResultLattice {
private:
    size_t rhs_;
    std::map<size_t, std::list<RawCFD>, std::greater<size_t>> layers_;

    bool HasChildrens(RawCFD const& node) const {
        size_t level_num = node.embedded_fd_.lhs_.count() - 1;
        if (!layers_.contains(level_num)) {
            return false;
        }
        for (auto const& bitset : cfdfinder::util::GenerateLhsSubsets(node.embedded_fd_.lhs_)) {
            for (auto const& result : layers_.at(level_num)) {
                if (result.embedded_fd_.lhs_ == bitset) {
                    return true;
                }
            }
        }
        return false;
    }

public:
    ResultLattice() = default;

    explicit ResultLattice(RawCFD root) : rhs_(root.embedded_fd_.rhs_) {
        Insert(std::move(root));
    }

    void Insert(RawCFD result) {
        size_t level = result.embedded_fd_.lhs_.count();
        layers_[level].push_back(std::move(result));
    }

    bool CanInsert(RawCFD const& cfd) const {
        return cfd.embedded_fd_.rhs_ == rhs_;
    }

    std::list<RawCFD> GetLeaves() {
        std::list<RawCFD> leaves;
        for (auto& layer : layers_) {
            for (auto& result : layer.second) {
                if (!HasChildrens(result)) {
                    leaves.push_back(std::move(result));
                }
            }
        }
        return leaves;
    }
};
}  // namespace algos::cfdfinder