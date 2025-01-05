#pragma once

#include "algorithms/md/decision_boundary.h"
#include "config/indices/type.h"
#include "model/types/numeric_type.h"
#include "model/types/string_type.h"

namespace algos::md {
using DecisionBoundary = model::md::DecisionBoundary;

class MDHighlights {
public:
    struct HighlightRecordInfo {
        config::IndexType column;
        std::byte const* first_value;
        std::byte const* second_value;
        model::TypeId type_id;
        DecisionBoundary similarity;
        DecisionBoundary decision_boundary;
    };

    struct HighlightRecord {
        std::pair<int, int> rows;
        HighlightRecordInfo info;
    };

private:
    std::vector<HighlightRecord> highlights;

    static std::string HighlightRecordAsString(HighlightRecord highlight);

public:
    MDHighlights() = default;
    std::vector<std::string> AsStrings() const;

    std::vector<HighlightRecord> const& GetRaw() const {
        return highlights;
    }

    void AddHighlight(std::pair<int, int> rows, HighlightRecordInfo info);
    void Reset();
};
}  // namespace algos::md
