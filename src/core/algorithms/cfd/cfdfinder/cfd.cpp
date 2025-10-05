#include "core/algorithms/cfd/cfdfinder/cfd.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>

namespace {
using namespace algos::cfdfinder;
using Condition = std::vector<std::string>;

Condition GetEntriesString(Pattern const& pattern,
                           InvertedClusterMaps const& inverted_cluster_maps) {
    Condition result;
    for (auto const& [id, entry] : pattern.GetEntries()) {
        InvertedClusterMap const& inverted_cluster_map = inverted_cluster_maps[id];
        result.push_back(entry->ToString(inverted_cluster_map));
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
