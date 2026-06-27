#include "core/algorithms/cfd/cfdfinder/model/expansion/range_pattern_expansion.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>

#include "core/algorithms/cfd/cfdfinder/model/pattern/range_entry.h"
#include "core/util/bitset_utils.h"

namespace algos::cfdfinder {

RangePatternExpansion::RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps,
                                             RowsPtr&& compressed_records)
    : ExpansionStrategy(std::move(compressed_records)) {
    sorted_clusters_ids_.reserve(inverted_cluster_maps.size());
    for (size_t i = 0; i < inverted_cluster_maps.size(); ++i) {
        std::vector<std::pair<ClusterId, AttributeValue>> mappings(inverted_cluster_maps[i].begin(),
                                                                   inverted_cluster_maps[i].end());

        std::ranges::sort(mappings, [](auto const& a, auto const& b) {
            if (b.second.empty()) {
                return false;
            }
            if (a.second.empty()) {
                return true;
            }
            return a.second < b.second;
        });

        std::vector<size_t> sorted_cluster_ids(mappings.size());
        std::ranges::transform(mappings, sorted_cluster_ids.begin(),
                               [](auto const& entry) { return entry.first; });

        sorted_clusters_ids_.push_back(
                std::make_shared<SortedClustersId const>(std::move(sorted_cluster_ids)));
    }
}

Entries RangePatternExpansion::GenerateNullEntries(BitSet const& attributes) const {
    Entries null_entries;
    util::ForEachIndex(attributes, [&](size_t attr) {
        auto const& clusters = sorted_clusters_ids_.at(attr);
        null_entries.emplace_back(attr,
                                  std::make_shared<RangeEntry>(clusters, 0, clusters->size() - 1));
    });

    return null_entries;
}

std::vector<RangePatternExpansion::RangeEntryPtr> RangePatternExpansion::GenerateReplacements(
        RangeEntry const& range_entry) const {
    std::vector<RangeEntryPtr> replacements;
    replacements.reserve(2);

    auto lentry = std::static_pointer_cast<RangeEntry>(range_entry.Clone());
    if (lentry->IncreaseLowerBound()) {
        replacements.push_back(std::move(lentry));
    }

    auto rentry = std::static_pointer_cast<RangeEntry>(range_entry.Clone());
    if (rentry->DecreaseUpperBound()) {
        replacements.push_back(std::move(rentry));
    }

    return replacements;
}

std::vector<RangePatternExpansion::RangeEntryPtr> RangePatternExpansion::FilterValidReplacements(
        Entries& entries, size_t replaced_index, std::vector<RangeEntryPtr>&& replacements,
        std::shared_ptr<PruningStrategy> pruning_strategy) const {
    std::vector<RangeEntryPtr> valid_entries;
    valid_entries.reserve(replacements.size());

    for (auto&& replacement : replacements) {
        if (IsValidReplacement(entries, replaced_index, replacement, *pruning_strategy)) {
            valid_entries.push_back(std::move(replacement));
        }
    }

    return valid_entries;
}

RangePatternExpansion::RangeCover RangePatternExpansion::CalculateCover(
        RangeEntry const& entry, std::vector<size_t> const& cluster_representatives,
        Cover const& cover) const {
    size_t support = 0;
    boost::dynamic_bitset<> cover_mask(cover.size());
    size_t const low = entry.GetLowerBound();
    size_t const high = entry.GetUpperBound();

    for (size_t cluster_id = 0; cluster_id < cover.size(); ++cluster_id) {
        size_t val = cluster_representatives[cluster_id];

        if (val >= low && val <= high) {
            support += cover[cluster_id].size();
            cover_mask.set(cluster_id);
        }
    }

    return {support, std::move(cover_mask)};
}

void RangePatternExpansion::Expand(Pattern&& parent_pattern, Frontier& frontier,
                                   Row const& inverted_pli_rhs,
                                   std::shared_ptr<PruningStrategy> pruning_strategy) const {
    auto parent_entries = parent_pattern.GetEntries();
    auto const& cover = parent_pattern.GetCover();

    for (size_t replaced_index = 0; replaced_index < parent_entries.size(); ++replaced_index) {
        auto const& item = parent_entries[replaced_index];
        auto const& range_entry = static_cast<RangeEntry const&>(*item.entry);
        auto replacements = GenerateReplacements(range_entry);
        auto valid_entries = FilterValidReplacements(parent_entries, replaced_index,
                                                     std::move(replacements), pruning_strategy);

        if (valid_entries.empty()) {
            continue;
        }

        std::vector<size_t> cluster_representatives = GetClusterRepresentatives(item.id, cover);

        for (auto const& new_entry : valid_entries) {
            auto [support, cover_mask] = CalculateCover(*new_entry, cluster_representatives, cover);

            if (pruning_strategy->IsPatternWorthConsidering(support)) {
                Entries child_entries = parent_entries;
                child_entries[replaced_index].entry = new_entry;

                Pattern child(std::move(child_entries), BuildCover(cover, cover_mask),
                              inverted_pli_rhs);
                frontier.Emplace(std::move(child));
            }
        }
    }
}

}  // namespace algos::cfdfinder
