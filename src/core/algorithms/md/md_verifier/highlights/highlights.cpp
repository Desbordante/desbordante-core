#include "algorithms/md/md_verifier/highlights/highlights.h"

#include <numeric>

namespace algos::md {
MDHighlights MDHighlights::CreateFrom(model::RhsSimilarityClassifierDesctription rhs_desc,
                                      RecordsPairsSet const& records_pairs,
                                      RecordsPairToSimilarityMap const& records_to_similarity) {
    size_t total_highlights_count =
            std::accumulate(records_pairs.begin(), records_pairs.end(), 0,
                            [](size_t acc, auto const& pair) { return acc + pair.second.size(); });
    std::vector<MDHighlights::Highlight> highlights;
    highlights.reserve(total_highlights_count);

    for (auto const& [left_record_index, right_records_set] : records_pairs) {
        for (model::Index right_record_index : right_records_set) {
            highlights.emplace_back(
                    left_record_index, right_record_index, rhs_desc,
                    records_to_similarity.at({left_record_index, right_record_index}));
        }
    }

    return MDHighlights(std::move(highlights));
}
}  // namespace algos::md
