#pragma once

#include <sstream>

#include "algorithms/md/md_verifier/md_verifier_column_match.h"
#include "algorithms/md/similarity.h"
#include "config/indices/type.h"
#include "model/index.h"

namespace algos::md {
class MDHighlights {
public:
    struct Highlight {
        model::Index left_table_row;
        model::Index right_table_row;
        MDVerifierColumnMatch column_match;
        model::md::Similarity similarity;
        model::md::DecisionBoundary decision_boundary;

        Highlight(model::Index left_table_row, model::Index right_table_row,
                  MDVerifierColumnMatch const& column_match, model::md::Similarity similarity,
                  model::md::DecisionBoundary decision_boundary)
            : left_table_row(left_table_row),
              right_table_row(right_table_row),
              column_match(column_match),
              similarity(similarity),
              decision_boundary(decision_boundary) {}

        std::string ToString() const {
            std::stringstream ss;
            ss << column_match.ToString() << " violates MD in " << left_table_row
               << " row of left table and " << right_table_row
               << " row of right table with similarity " << similarity << " and decision boundary "
               << decision_boundary;
            return ss.str();
        };
    };

private:
    std::vector<Highlight> highlights_;

public:
    void RegisterHighlight(model::Index left_table_row, model::Index right_table_row,
                           MDVerifierColumnMatch const& column_match,
                           model::md::Similarity similarity,
                           model::md::DecisionBoundary decision_boundary);

    std::vector<Highlight> const& GetHighlights() const {
        return highlights_;
    }
};
}  // namespace algos::md
