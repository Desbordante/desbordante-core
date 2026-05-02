#include "core/algorithms/cfd/cfun/attribute_utils.h"

#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfun/quadruple.h"
#include "core/util/bitset_utils.h"

namespace algos::cfd::cfun {
AttributeIndex::AttributeIndex(std::vector<Quadruple> const& level) {
    index_.reserve(level.size());
    for (auto const& candidate : level) {
        index_.emplace(candidate.GetAttributes(), &candidate);
    }
}

bool AttributeIndex::AllSubsetsContains(Quadruple::AttributeSet attributes) const {
    auto subset = attributes;

    for (auto attr = attributes.find_first(); attr != boost::dynamic_bitset<>::npos;
         attr = attributes.find_next(attr)) {
        subset.reset(attr);

        if (!index_.contains(subset)) {
            return false;
        }
        subset.set(attr);
    }

    return true;
}

std::vector<Quadruple const*> AttributeIndex::GetAllSubsets(Quadruple const& candidate) const {
    auto const& attributes = candidate.GetAttributes();
    std::vector<Quadruple const*> subsets;
    subsets.reserve(attributes.size());

    auto subset = attributes;

    util::ForEachIndex(attributes, [&](auto attr) {
        subset.reset(attr);

        if (auto it = index_.find(subset); it != index_.end()) {
            subsets.push_back(it->second);
        }
        subset.set(attr);
    });

    return subsets;
}
}  // namespace algos::cfd::cfun
