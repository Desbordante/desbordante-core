#include "algorithms/cfd/cfdfinder/model/expansion/constant_expansion_strategy.h"

#include <cstddef>

#include "algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "algorithms/cfd/cfdfinder/model/pattern/variable_entry.h"
#include "util/bitset_utils.h"

namespace algos::cfdfinder {

Pattern ConstantExpansion::GenerateNullPattern(BitSet const& attributes) const {
    Entries entries;
    util::ForEachIndex(attributes, [&entries](size_t attr) {
        entries.emplace_back(attr, std::make_shared<VariableEntry>());
    });

    return Pattern(std::move(entries));
}

std::list<Pattern> ConstantExpansion::GetChildPatterns(Pattern const& current_pattern) const {
    std::list<Pattern> result;
    for (auto const& cluster : current_pattern.GetCover()) {
        result.splice(result.end(), GetChildPatterns(current_pattern, cluster));
    }
    return result;
}

std::list<Pattern> ConstantExpansion::GetChildPatterns(Pattern const& pattern,
                                                       Cluster const& cluster) const {
    std::list<Pattern> results;
    auto const& entries = pattern.GetEntries();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].entry->IsConstant()) {
            continue;
        }
        int value = (*compressed_records_)[cluster[0]][entries[i].id];

        auto new_entries = entries;
        new_entries[i].entry = std::make_shared<ConstantEntry>(value);
        results.emplace_back(std::move(new_entries));
    }

    return results;
}

}  // namespace algos::cfdfinder
