#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/minimize_tree.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/dif_func_info.h"
#include "core/algorithms/dd/fastdd/util/static_bitset.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class TranslatingMinimizeTree {
private:
    std::shared_ptr<DifFuncInfo<Bitset> const> dif_func_info_;
    MinimizeTree<Bitset> tree_;

    std::vector<std::size_t> TransformToNodes(Bitset const& bitset) const {
        std::vector<std::size_t> nodes;
        nodes.reserve(bitset.count());
        for (std::size_t index = bitset.find_first(); index != Bitset::npos;
             index = bitset.find_next(index)) {
            nodes.push_back(dif_func_info_->dif_func_to_column_index_[index]);
        }

        return nodes;
    }

    Bitset TransformToBitset(Bitset const& bitset) const {
        Bitset transformed(bitset.size());
        transformed.flip();

        for (std::size_t index = bitset.find_first(); index != Bitset::npos;
             index = bitset.find_next(index)) {
            transformed &= dif_func_info_->dif_func_to_bitset_[index];
        }

        return transformed;
    }

public:
    explicit TranslatingMinimizeTree(std::shared_ptr<DifFuncInfo<Bitset> const> dif_func_info)
        : dif_func_info_(dif_func_info), tree_(dif_func_info_->dif_func_num_) {}

    std::vector<Bitset> Minimize(std::vector<Bitset> candidates) {
        std::ranges::sort(candidates, [](Bitset const& a, Bitset const& b) {
            int diff = a.count() - b.count();
            return diff != 0 ? diff < 0 : a < b;
        });

        std::vector<Bitset> result;
        for (auto const& candidate : candidates) {
            bool superset_found =
                    tree_.Add(TransformToBitset(candidate), TransformToNodes(candidate));
            if (!superset_found) {
                result.push_back(candidate);
            }
        }

        return result;
    }
};

}  // namespace algos::dd
