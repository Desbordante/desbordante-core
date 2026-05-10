#include "core/algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"

#include <algorithm>
#include <cstddef>
#include <numeric>

#include "core/algorithms/md/hymd/lowest_bound.h"
#include "core/model/index.h"
#include "core/model/types/string_type.h"
#include "core/util/desbordante_assume.h"
#include "core/util/levenshtein_distance.h"

namespace algos::hymd::preprocessing::column_matches {

std::size_t detail::LevenshteinComparerCreator::GetLargestStringSize(
        std::vector<model::String> const& elements) {
    DESBORDANTE_ASSUME(!elements.empty());
    return std::ranges::max(elements | std::views::transform(std::mem_fn(&model::String::size)));
}

preprocessing::Similarity detail::LevenshteinComparerCreator::Comparer::operator()(
        model::String const& l, model::String const& r) {
    std::size_t const max_dist = std::max(l.size(), r.size());
    Similarity similarity = 1.0;
    if (max_dist != 0) {
        std::size_t lim = max_dist * (1 - min_sim_);
        std::size_t dist = util::LevenshteinDistance(&l, &r, buf.get(), r_buf, lim, max_dist);
        similarity = (max_dist - dist) / static_cast<Similarity>(max_dist);
        if (similarity < min_sim_) similarity = kLowestBound;
    }
    return similarity;
}

Levenshtein::Levenshtein(ColumnIdentifier left_column_identifier,
                         ColumnIdentifier right_column_identifier,
                         model::md::DecisionBoundary min_sim,
                         ccv_id_pickers::SimilaritiesPicker picker,
                         detail::LevenshteinTransformer::TransformFunctionsOption funcs)
    : detail::LevenshteinBase(true, kName, std::move(left_column_identifier),
                              std::move(right_column_identifier), {std::move(funcs)},
                              {min_sim, std::move(picker)}) {}

Levenshtein::Levenshtein(ColumnIdentifier left_column_identifier,
                         ColumnIdentifier right_column_identifier,
                         model::md::DecisionBoundary min_sim, std::size_t size_limit,
                         detail::LevenshteinTransformer::TransformFunctionsOption funcs)
    : detail::LevenshteinBase(true, kName, std::move(left_column_identifier),
                              std::move(right_column_identifier), {std::move(funcs)},
                              {min_sim, ccv_id_pickers::IndexUniform<Similarity>(size_limit)}) {}

}  // namespace algos::hymd::preprocessing::column_matches
