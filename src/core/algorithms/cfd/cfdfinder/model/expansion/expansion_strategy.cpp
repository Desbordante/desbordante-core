#include "core/algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"
#include "core/util/bitset_utils.h"

namespace algos::cfdfinder {

ExpansionStrategy::Cover ExpansionStrategy::BuildCover(Cover const& cover,
                                                       BitSet const& cover_mask) {
    Cover child_cover;
    child_cover.reserve(cover_mask.count());
    util::ForEachIndex(cover_mask,
                       [&](size_t cluster_id) { child_cover.push_back(cover[cluster_id]); });
    return child_cover;
}

bool ExpansionStrategy::IsValidReplacement(Entries& entries, size_t replaced_index,
                                           std::shared_ptr<Entry> replacement,
                                           PruningStrategy& pruning_strategy) {
    std::shared_ptr<Entry>& original_entry = entries[replaced_index].entry;

    std::swap(original_entry, replacement);
    bool const valid = pruning_strategy.ValidForProcessing(entries);
    std::swap(original_entry, replacement);

    return valid;
}

std::vector<size_t> ExpansionStrategy::GetClusterRepresentatives(size_t id,
                                                                 Cover const& cover) const {
    auto const& column = columns_values_.GetColumn(id);

    std::vector<size_t> cluster_representatives;
    cluster_representatives.reserve(cover.size());
    std::transform(cover.begin(), cover.end(), std::back_inserter(cluster_representatives),
                   [&](auto const& cluster) { return column[cluster[0]]; });

    return cluster_representatives;
}

Pattern ExpansionStrategy::GenerateNullPattern(BitSet const& lhs_attributes,
                                               model::PLI const* lhs_pli,
                                               Row const& inverted_pli_rhs) const {
    auto null_entries = GenerateNullEntries(lhs_attributes);
    auto null_cover = utils::EnrichPLI(lhs_pli, columns_values_.GetColumn(0).size());

    return Pattern(std::move(null_entries), std::move(null_cover), inverted_pli_rhs);
}

}  // namespace algos::cfdfinder
