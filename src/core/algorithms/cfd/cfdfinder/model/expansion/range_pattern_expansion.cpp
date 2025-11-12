#include "range_pattern_expansion.h"

#include "algorithms/cfd/cfdfinder/model/pattern/range_entry.h"

namespace algos::cfdfinder {

RangePatternExpansion::RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps) {
    sorted_clusters_ids_.reserve(inverted_cluster_maps.size());
    for (size_t i = 0; i < inverted_cluster_maps.size(); i++) {
        std::vector<std::pair<ClusterId, AttributeValue>> mappings(inverted_cluster_maps[i].begin(),
                                                                   inverted_cluster_maps[i].end());

        std::sort(mappings.begin(), mappings.end(), [](auto const& a, auto const& b) {
            if (a.second.empty() && b.second.empty()) {
                return false;
            }
            if (a.second.empty()) {
                return true;
            }
            if (b.second.empty()) {
                return false;
            }
            return a.second < b.second;
        });

        SortedClustersId sorted_cluster_ids;
        for (auto const& entry : mappings) {
            sorted_cluster_ids.push_back(entry.first);
        }
        sorted_clusters_ids_.push_back(
                std::make_shared<SortedClustersId>(std::move(sorted_cluster_ids)));
    }
}

Pattern RangePatternExpansion::GenerateNullPattern(BitSet const& attributes) {
    Entries entries;

    for (size_t i = attributes.find_first(); i != BitSet::npos; i = attributes.find_next(i)) {
        auto const& clusters = sorted_clusters_ids_.at(i);
        entries.emplace_back(i, std::make_shared<RangeEntry>(clusters, 0, clusters->size() - 1));
    }
    return Pattern(std::move(entries));
}

std::list<Pattern> RangePatternExpansion::GetChildPatterns(Pattern const& current_pattern) {
    std::list<Pattern> result;
    auto const& entries = current_pattern.GetEntries();

    for (size_t i = 0; i < entries.size(); ++i) {
        auto range_entry = static_cast<RangeEntry const*>(entries[i].entry.get());

        auto lentry = std::static_pointer_cast<RangeEntry>(range_entry->Clone());
        if (lentry->IncreaseLowerBound()) {
            auto new_entries = entries;
            new_entries[i].entry = lentry;
            result.emplace_back(std::move(new_entries));
        }

        auto rentry = std::static_pointer_cast<RangeEntry>(range_entry->Clone());
        if (rentry->DecreaseUpperBound()) {
            auto new_entries = entries;
            new_entries[i].entry = rentry;
            result.emplace_back(std::move(new_entries));
        }
    }

    return result;
}

}  // namespace algos::cfdfinder