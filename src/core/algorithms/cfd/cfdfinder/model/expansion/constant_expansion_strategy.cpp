#include "constant_expansion_strategy.h"

#include "algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "algorithms/cfd/cfdfinder/model/pattern/variable_entry.h"

namespace algos::cfdfinder {

Pattern ConstantExpansion::GenerateNullPattern(boost::dynamic_bitset<> const& attributes) {
    Entries entries;
    for (size_t i = attributes.find_first(); i != boost::dynamic_bitset<>::npos;
         i = attributes.find_next(i)) {
        entries.emplace_back(i, std::make_shared<VariableEntry>());
    }
    return Pattern(std::move(entries));
}

std::list<Pattern> ConstantExpansion::GetChildPatterns(Pattern const& current_pattern) {
    std::list<Pattern> result;
    for (auto const& cluster : current_pattern.GetCover()) {
        result.splice(result.end(), GetChildPatterns(current_pattern, cluster));
    }
    return result;
}

std::list<Pattern> ConstantExpansion::GetChildPatterns(Pattern const& pattern,
                                                       Cluster const& cluster) {
    std::list<Pattern> results;
    auto const& entries = pattern.GetEntries();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].entry->GetType() != EntryType::kVariable) {
            continue;
        }
        int value = compressed_records_->at(cluster[0])[entries[i].id];

        auto new_entries = entries;
        new_entries[i].entry = std::make_shared<ConstantEntry>(value);
        results.emplace_back(std::move(new_entries));
    }

    return results;
}

}  // namespace algos::cfdfinder