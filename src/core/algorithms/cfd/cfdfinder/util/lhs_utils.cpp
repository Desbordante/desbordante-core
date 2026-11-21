#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"

#include <cstddef>
#include <list>

#include "core/util/bitset_utils.h"

namespace algos::cfdfinder::utils {

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs) {
    std::list<BitSet> subsets;

    util::ForEachIndex(lhs, [&](size_t attr) {
        auto subset = lhs;
        subset.flip(attr);

        if (subset.any()) {
            subsets.push_back(std::move(subset));
        }
    });

    return subsets;
}

std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs) {
    std::list<BitSet> supersets;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (!lhs.test(i)) {
            BitSet superset(lhs);
            superset.set(i);
            supersets.push_back(std::move(superset));
        }
    }
    return supersets;
}

std::vector<Cluster> EnrichPLI(model::PLI const* lhs_pli, size_t num_tuples) {
    std::unordered_set<int> existing_elements;
    existing_elements.reserve(lhs_pli->GetSize());

    auto const& original_clusters = lhs_pli->GetIndex();
    for (auto const& cluster : original_clusters) {
        existing_elements.insert(cluster.begin(), cluster.end());
    }
    size_t missing_count = num_tuples - existing_elements.size();

    std::vector<Cluster> enriched_clusters;
    enriched_clusters.reserve(original_clusters.size() + missing_count);
    enriched_clusters.assign(original_clusters.begin(), original_clusters.end());

    for (size_t i = 0; i < num_tuples; ++i) {
        if (!existing_elements.contains(i)) {
            enriched_clusters.emplace_back(1, i);
        }
    }
    return enriched_clusters;
}
}  // namespace algos::cfdfinder::utils
