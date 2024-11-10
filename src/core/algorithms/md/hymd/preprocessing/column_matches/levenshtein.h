#pragma once

#include <memory>
#include <vector>

#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "algorithms/md/hymd/preprocessing/column_matches/basic_calculator.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match_impl.h"
#include "algorithms/md/hymd/preprocessing/column_matches/single_transformer.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/utility/make_unique_for_overwrite.h"
#include "model/types/builtin.h"

namespace algos::hymd::preprocessing::column_matches {
namespace detail {
class LevenshteinComparerCreator {
    struct Comparer {
        std::unique_ptr<unsigned[]> buf;
        unsigned* r_buf;
        preprocessing::Similarity min_sim_;

        preprocessing::Similarity operator()(model::String const& l, model::String const& r);
    };

    preprocessing::Similarity min_sim_;
    std::size_t const buf_len_;

    static std::size_t GetLargestStringSize(std::vector<model::String> const& elements);

public:
    LevenshteinComparerCreator(preprocessing::Similarity min_sim,
                               std::vector<model::String> const* left_elements)
        : min_sim_(min_sim), buf_len_(GetLargestStringSize(*left_elements) + 1) {}

    Comparer operator()() const {
        // TODO: replace with std::make_unique_for_overwrite when GCC in CI is upgraded
        auto buf = utility::MakeUniqueForOverwrite<unsigned[]>(buf_len_ * 2);
        auto* buf_ptr = buf.get();
        return {std::move(buf), buf_ptr + buf_len_, min_sim_};
    }
};

class LevenshteinComparerCreatorSupplier {
    preprocessing::Similarity min_sim_;

public:
    LevenshteinComparerCreatorSupplier(preprocessing::Similarity min_sim) : min_sim_(min_sim) {}

    LevenshteinComparerCreator operator()(std::vector<model::String> const* left_elements,
                                          std::vector<model::String> const*,
                                          indexes::KeyedPositionListIndex const&) const {
        return {min_sim_, left_elements};
    }
};

using LevenshteinTransformer = TypeTransformer<model::String>;

using LevenshteinBase =
        ColumnMatchImpl<LevenshteinTransformer,
                        BasicCalculator<LevenshteinComparerCreatorSupplier, true, true>>;
}  // namespace detail

class Levenshtein final : public detail::LevenshteinBase {
    static constexpr auto kName = "levenshtein";

public:
    using TransformFunctionsOption = detail::LevenshteinTransformer::TransformFunctionsOption;

    Levenshtein(ColumnIdentifier left_column_identifier, ColumnIdentifier right_column_identifier,
                model::md::DecisionBoundary min_sim, ccv_id_pickers::SimilaritiesPicker picker,
                TransformFunctionsOption funcs);

    Levenshtein(ColumnIdentifier left_column_identifier, ColumnIdentifier right_column_identifier,
                model::md::DecisionBoundary min_sim, std::size_t size_limit = 0,
                TransformFunctionsOption funcs = {});
};

}  // namespace algos::hymd::preprocessing::column_matches
