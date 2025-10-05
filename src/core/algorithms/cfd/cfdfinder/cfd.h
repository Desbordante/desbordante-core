#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_tableau.h"
#include "core/algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "core/algorithms/fd/fd.h"

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
    CFD(FD embedded_fd, Tableau tableau, double support, double confidence)
        : embedded_fd_(std::move(embedded_fd)),
          patterns_(std::move(tableau)),
          support_(support),
          confidence_(confidence) {}

    CFD(Vertical lhs, Column rhs, PatternTableau const& tableau,
        std::shared_ptr<RelationalSchema const> schema,
        InvertedClusterMaps const& inverted_cluster_maps);

    std::string ToString() const;

    bool operator==(CFD const& other) const {
        return support_ == other.support_ && confidence_ == other.confidence_ &&
               patterns_ == other.patterns_ &&
               embedded_fd_.ToNameTuple() == other.embedded_fd_.ToNameTuple();
    }

    bool operator!=(CFD const& other) const {
        return !(*this == other);
    }

    double GetSupport() const noexcept {
        return support_;
    }

    double GetConfidence() const noexcept {
        return confidence_;
    }

    FD const& GetEmbeddedFD() const noexcept {
        return embedded_fd_;
    }

    Tableau const& GetTableau() const noexcept {
        return patterns_;
    }
};
}  // namespace algos::cfdfinder
