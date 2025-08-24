#pragma once

#include <type_traits>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/column_matches/basic_calculator.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match_impl.h"
#include "algorithms/md/hymd/preprocessing/column_matches/single_transformer.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "model/types/builtin.h"
#include "util/argument_type.h"

namespace algos::hymd::preprocessing::column_matches {
namespace detail {
template <auto Function>
class BasicComparerCreator {
    struct Comparer {
        preprocessing::Similarity min_sim_;
        using LType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>;
        using RType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>;

        preprocessing::Similarity operator()(LType const& l, RType const& r) {
            preprocessing::Similarity sim = Function(l, r);
            return sim < min_sim_ ? kLowestBound : sim;
        }
    };

    preprocessing::Similarity min_sim_;

public:
    explicit BasicComparerCreator(preprocessing::Similarity min_sim) : min_sim_(min_sim) {}

    Comparer operator()() const {
        return {min_sim_};
    }
};

template <auto Function>
class BasicComparerCreatorSupplier {
    using LeftElementType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>;
    using RightElementType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>;
    preprocessing::Similarity min_sim_;

public:
    BasicComparerCreatorSupplier(preprocessing::Similarity min_sim) : min_sim_(min_sim) {}

    auto operator()(std::vector<LeftElementType> const*, std::vector<RightElementType> const*,
                    indexes::KeyedPositionListIndex const&) const {
        return BasicComparerCreator<Function>{min_sim_};
    }
};

template <auto Function>
using PairwiseBaseTypeTransformer =
        TypeTransformer<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                        std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>>;

template <auto Function, bool... Params>
using PairwiseBase =
        ColumnMatchImpl<PairwiseBaseTypeTransformer<Function>,
                        BasicCalculator<BasicComparerCreatorSupplier<Function>, Params...>>;
}  // namespace detail

template <auto SimilarityMeasure, bool Symmetric, bool EqMax, bool... Params>
class Pairwise : public detail::PairwiseBase<SimilarityMeasure, Symmetric, EqMax, Params...> {
public:
    using TransformFunctionsOption =
            detail::PairwiseBaseTypeTransformer<SimilarityMeasure>::TransformFunctionsOption;

    Pairwise(std::string name, ColumnIdentifier left_column_identifier,
             ColumnIdentifier right_column_identifier, model::md::DecisionBoundary min_sim,
             ccv_id_pickers::SimilaritiesPicker picker, TransformFunctionsOption funcs = {})
        : detail::PairwiseBase<SimilarityMeasure, Symmetric, EqMax, Params...>(
                  Symmetric && EqMax, std::move(name), std::move(left_column_identifier),
                  std::move(right_column_identifier), {std::move(funcs)},
                  {{min_sim}, std::move(picker)}) {};

    Pairwise(std::string name, ColumnIdentifier left_column_identifier,
             ColumnIdentifier right_column_identifier, model::md::DecisionBoundary min_sim,
             std::size_t size_limit = 0, TransformFunctionsOption funcs = {})
        : Pairwise(std::move(name), std::move(left_column_identifier),
                   std::move(right_column_identifier), min_sim,
                   ccv_id_pickers::IndexUniform<Similarity>(size_limit), std::move(funcs)) {};
};

template <auto SimilarityMeasure>
using NormalPairwise = Pairwise<SimilarityMeasure, true, true, true>;
}  // namespace algos::hymd::preprocessing::column_matches
