#include "cfd.h"

#include "cfd/cfdfinder/model/entries.h"

namespace {
using namespace algos::cfdfinder;
using Condition = std::vector<std::string>;

Condition GetEntriesString(Pattern const& pattern,
                           InvertedClusterMaps const& inverted_cluster_maps) {
    Condition result;
    static std::string const kNullRepresentation = "null";
    static std::string const kNegationSign = "¬";
    static std::string const kWildCard = "_";

    for (auto const& [id, entry] : pattern.GetEntries()) {
        auto const& inverted_cluster_map = inverted_cluster_maps[id];
        switch (entry->GetType()) {
            case EntryType::kVariable:
                result.push_back(kWildCard);
                break;
            case EntryType::kConstant: {
                auto const* constant_entry = static_cast<ConstantEntry const*>(entry.get());
                std::string value =
                        inverted_cluster_map.find(constant_entry->GetConstant())->second;
                if (value.empty()) {
                    value = kNullRepresentation;
                }

                result.push_back(std::move(value));
                break;
            }
            case EntryType::kNegativeConstant: {
                auto const* neg_constant_entry =
                        static_cast<NegativeConstantEntry const*>(entry.get());
                std::string value =
                        inverted_cluster_map.find(neg_constant_entry->GetConstant())->second;

                value = (!value.empty()) ? kNegationSign + value
                                         : kNegationSign + kNullRepresentation;
                result.push_back(std::move(value));
                break;
            }
            case EntryType::kRange: {
                auto const* range_entry = static_cast<RangeEntry const*>(entry.get());

                std::string lower_bound;
                std::string upper_bound;

                lower_bound = inverted_cluster_map.find(range_entry->GetLowerBound())->second;
                upper_bound = inverted_cluster_map.find(range_entry->GetUpperBound())->second;
                // if (range_entry->GetLowerBound() == range_entry->GetUpperBound()) {
                //     result.push_back("[" + lower_bound + "]");
                //     break;
                // }
                if (lower_bound.empty()) {
                    lower_bound = kNullRepresentation;
                }
                if (upper_bound.empty()) {
                    upper_bound = kNullRepresentation;
                }

                result.push_back("[" + lower_bound + " - " + upper_bound + "]");
                break;
            }
        }
    }

    return result;
}
}  // namespace

namespace algos::cfdfinder {

CFD::CFD(Vertical lhs, Column rhs, PatternTableau const& tableau,
         std::shared_ptr<RelationalSchema const> schema,
         InvertedClusterMaps const& inverted_cluster_maps)
    : embedded_fd_(std::move(lhs), std::move(rhs), std::move(schema)) {
    support_ = tableau.GetSupport();
    confidence_ = tableau.GetConfidence();
    patterns_.reserve(tableau.GetPatterns().size());
    for (auto const& pattern : tableau.GetPatterns()) {
        Condition condition = GetEntriesString(pattern, inverted_cluster_maps);
        patterns_.push_back(std::move(condition));
    }
}

std::string CFD::ToString() const {
    std::ostringstream oss;
    oss << embedded_fd_.ToLongString();
    oss << "\nPatternTableau {\n";

    for (auto const& pattern : patterns_) {
        oss << "\t(" << boost::algorithm::join(pattern, "|") << ")\n";
    }
    oss << "}\n";
    return oss.str();
}
}  // namespace algos::cfdfinder