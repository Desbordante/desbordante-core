#pragma once

#include <cstddef>
#include <list>
#include <map>
#include <ranges>

#include "core/algorithms/cfd/cfdfinder/model/result/raw_cfd.h"
#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"

namespace algos::cfdfinder {
class ResultLattice {
private:
    size_t rhs_;
    std::map<size_t, std::list<RawCFD>, std::greater<size_t>> layers_;

    bool HasChildren(RawCFD const& node) const {
        size_t level_num = node.embedded_fd_.lhs_.count() - 1;
        if (!layers_.contains(level_num)) {
            return false;
        }

        auto const& level = layers_.at(level_num);
        auto subsets = cfdfinder::utils::GenerateLhsSubsets(node.embedded_fd_.lhs_);

        return std::ranges::any_of(subsets, [&level](auto const& subset) {
            return std::ranges::any_of(level, [&subset](auto const& result) {
                return result.embedded_fd_.lhs_ == subset;
            });
        });
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
                if (!HasChildren(result)) {
                    leaves.push_back(std::move(result));
                }
            }
        }
        return leaves;
    }
};
}  // namespace algos::cfdfinder
