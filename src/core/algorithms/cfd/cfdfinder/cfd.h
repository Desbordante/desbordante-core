#pragma once

#include <variant>

#include <boost/algorithm/string/join.hpp>

#include "algorithms/fd/fd.h"
#include "model/pattern/pattern_tableau.h"

namespace algos::cfdfinder {
class CFD {
private:
    using Entry = std::string;
    using Pattern = std::vector<Entry>;
    using Tableau = std::vector<Pattern>;

    FD embedded_fd_;
    Tableau patterns_;
    double support_;
    double confidence_;

public:
    CFD(Vertical lhs, Column rhs, PatternTableau const& tableau,
        std::shared_ptr<RelationalSchema const> schema, Tableau patterns_values)
        : embedded_fd_(std::move(lhs), std::move(rhs), std::move(schema)),
          patterns_(std::move(patterns_values)) {
        support_ = tableau.GetSupport();
        confidence_ = tableau.GetConfidence();
    }

    std::string ToString() const {
        std::ostringstream oss;
        oss << embedded_fd_.ToLongString();
        oss << "\nPatternTableau {\n";

        for (auto const& pattern : patterns_) {
            oss << "\t(" << boost::algorithm::join(pattern, "|") << ")\n";
        }
        oss << "}\n";
        return oss.str();
    }

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