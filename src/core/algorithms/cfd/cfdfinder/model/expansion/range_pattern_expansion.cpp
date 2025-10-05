#include "range_pattern_expansion.h"

#include <list>

#include <boost/container/flat_map.hpp>

#include "algorithms/cfd/cfdfinder/model/pattern/range_entry.h"

namespace algos::cfdfinder {

RangePatternExpansion::RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps) {
    size_t index = 0;
    for (auto const& inverted_cluster_map : inverted_cluster_maps) {
        std::vector<std::pair<size_t, std::string>> mappings(inverted_cluster_map.begin(),
                                                             inverted_cluster_map.end());

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

        std::vector<size_t> sorted_cluster_ids;
        for (auto const& entry : mappings) {
            sorted_cluster_ids.push_back(entry.first);
        }

        sorted_clusters_ids_[index++] = std::move(sorted_cluster_ids);
    }
}

Pattern RangePatternExpansion::GenerateNullPattern(boost::dynamic_bitset<> const& attributes) {
    Entries entries;

    for (size_t i = attributes.find_first(); i != boost::dynamic_bitset<>::npos;
         i = attributes.find_next(i)) {
        auto const& clusters = sorted_clusters_ids_.at(i);
        entries.emplace_back(i, std::make_shared<RangeEntry>(clusters, 0, clusters.size() - 1));
    }
    return Pattern(std::move(entries));
}

std::list<Pattern> RangePatternExpansion::GetChildPatterns(Pattern const& current_pattern) {
    std::list<Pattern> result;
    auto const& entries = current_pattern.GetEntries();

    for (size_t i = 0; i < entries.size(); ++i) {
        auto range_entry = std::dynamic_pointer_cast<RangeEntry>(entries[i].entry);

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