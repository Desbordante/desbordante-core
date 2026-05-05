#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/minimize_tree.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/dif_func_info.h"

namespace algos::dd {

class TranslatingMinimizeTree {
private:
    MinimizeTree tree_;
    std::shared_ptr<DifFuncInfo const> dif_func_info_;

    template <BoostDynamicBitsetCompatible Bitset>
    std::vector<std::size_t> TransformToNodes(Bitset const& bitset) const {
        std::vector<std::size_t> nodes;
        nodes.reserve(bitset.count());
        for (std::size_t index = bitset.find_first(); index != Bitset::npos;
             index = bitset.find_next(index)) {
            nodes.push_back(dif_func_info_->dif_func_to_column_index_[index]);
        }

        return nodes;
    }

    template <BoostDynamicBitsetCompatible Bitset>
    boost::dynamic_bitset<> TransformToBitset(Bitset const& bitset) const {
        boost::dynamic_bitset<> transformed(bitset.size());
        transformed.flip();

        for (std::size_t index = bitset.find_first(); index != Bitset::npos;
             index = bitset.find_next(index)) {
            transformed &= dif_func_info_->dif_func_to_bitset_[index];
        }

        return transformed;
    }

public:
    explicit TranslatingMinimizeTree(std::shared_ptr<DifFuncInfo const> dif_func_info)
        : dif_func_info_(dif_func_info) {}

    template <BoostDynamicBitsetCompatible Bitset>
    std::vector<Bitset> Minimize(std::vector<Bitset> candidates) {
        std::ranges::sort(candidates, [](Bitset const& a, Bitset const& b) {
            int diff = a.count() - b.count();
            return diff != 0 ? diff < 0 : a < b;
        });

        std::vector<Bitset> result;
        for (auto const& candidate : candidates) {
            std::optional<boost::dynamic_bitset<>> superset =
                    tree_.Add(TransformToBitset(candidate), TransformToNodes(candidate));
            if (!superset) {
                result.push_back(candidate);
            }
        }

        return result;
    }
};

}  // namespace algos::dd
