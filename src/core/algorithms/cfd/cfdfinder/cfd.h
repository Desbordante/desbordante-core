#pragma once

#include <boost/algorithm/string/join.hpp>

#include "algorithms/fd/fd.h"
#include "cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "model/pattern/pattern_tableau.h"

namespace algos::cfdfinder {
class CFD {
private:
    using Entry = std::string;
    using Condition = std::vector<Entry>;
    using Tableau = std::vector<Condition>;

    FD embedded_fd_;
    Tableau patterns_;
    double support_;
    double confidence_;

public:
    CFD(Vertical lhs, Column rhs, PatternTableau const& tableau,
        std::shared_ptr<RelationalSchema const> schema,
        InvertedClusterMaps const& inverted_cluster_maps);

    std::string ToString() const;

    double GetSupport() const {
        return support_;
    }

    double GetConfidence() const {
        return confidence_;
    }

    FD const& GetEmbeddedFD() const {
        return embedded_fd_;
    }

    Tableau const& GetTableau() const {
        return patterns_;
    }
};
}  // namespace algos::cfdfinder