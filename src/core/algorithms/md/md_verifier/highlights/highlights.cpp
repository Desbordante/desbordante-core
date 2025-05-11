#include "algorithms/md/md_verifier/highlights/highlights.h"

namespace algos::md {
MDHighlights MDHighlights::CreateFrom(model::RhsSimilarityClassifierDesctription rhs_desc,
                                      RowsPairSet const& rows_pairs,
                                      RowsToSimilarityMap const& rows_to_similarity) {
    MDHighlights highlights;
    for (auto [left_row, right_rows_set] : rows_pairs) {
        for (auto right_row : right_rows_set) {
            highlights.highlights_.emplace_back(left_row, right_row, rhs_desc,
                                                rows_to_similarity.at({left_row, right_row}));
        }
    }

    return highlights;
}
}  // namespace algos::md
