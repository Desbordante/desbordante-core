#pragma once

#include "algorithms/md/decision_boundary.h"
#include "config/indices/type.h"
#include "model/types/numeric_type.h"
#include "model/types/string_type.h"

namespace algos::md {
using DecisionBoundary = model::md::DecisionBoundary;

class MDHighlights {
public:
    struct HighlightRecord {
        std::pair<int, int> rows;
        config::IndexType column;
        std::string first_value;
        std::string second_value;
        DecisionBoundary similarity;
        DecisionBoundary decision_boundary;

        bool operator==(HighlightRecord const& other) const = default;
    };

private:
    std::vector<HighlightRecord> highlights_;

public:
    MDHighlights() = default;

    std::vector<HighlightRecord> const& GetHighlights() const {
        return highlights_;
    }

    void AddHighlight(HighlightRecord record);
    void Reset();
};
}  // namespace algos::md
