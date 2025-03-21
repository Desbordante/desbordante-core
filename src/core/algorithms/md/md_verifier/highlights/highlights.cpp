#include "algorithms/md/md_verifier/highlights/highlights.h"

namespace algos::md {
void MDHighlights::RegisterHighlight(model::Index left_table_row, model::Index right_table_row,
                                     MDVerifierColumnMatch const& column_match,
                                     model::md::Similarity similarity,
                                     model::md::DecisionBoundary decision_boundary) {
    highlights_.push_back(
            {left_table_row, right_table_row, column_match, similarity, decision_boundary});
}
}  // namespace algos::md
