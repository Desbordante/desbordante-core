#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/minimize_tree.h"
#include "core/algorithms/dd/fastdd/util/dif_func_info.h"

namespace algos::dd {

class TranslatingMinimizeTree {
private:
    MinimizeTree tree_;
    std::shared_ptr<DifFuncInfo const> dif_func_info_;

    std::vector<std::size_t> TransformToNodes(boost::dynamic_bitset<> const& bitset) const;
    boost::dynamic_bitset<> TransformToBitset(boost::dynamic_bitset<> const& bitset) const;

public:
    explicit TranslatingMinimizeTree(std::shared_ptr<DifFuncInfo const> dif_func_info)
        : dif_func_info_(dif_func_info) {}

    std::unordered_set<boost::dynamic_bitset<>> Minimize(
            std::vector<boost::dynamic_bitset<>> candidates);
};

}  // namespace algos::dd
