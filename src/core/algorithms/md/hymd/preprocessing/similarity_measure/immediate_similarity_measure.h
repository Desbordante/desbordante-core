#pragma once

#include <type_traits>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/basic_calculator.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/column_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/single_transformer.h"
#include "model/types/builtin.h"
#include "util/argument_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
using SimilarityFunction = std::function<Similarity(std::byte const*, std::byte const*)>;

namespace detail {
template <auto Function>
class BasicComparerCreator {
    struct Comparer {
        preprocessing::Similarity min_sim_;
        using T = util::ArgumentType<decltype(Function), 0>;

        preprocessing::Similarity operator()(T const& l, T const& r) {
            preprocessing::Similarity sim = Function(l, r);
            return sim < min_sim_ ? kLowestBound : sim;
        }
    };

    preprocessing::Similarity min_sim_;

public:
    template <typename... Args>
    BasicComparerCreator(preprocessing::Similarity min_sim, Args&&...) : min_sim_(min_sim) {}

    Comparer operator()() const {
        return {min_sim_};
    }
};

template <auto Function>
using ImmediateBaseTypeTransformer =
        TypeTransformer<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                        std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>>;

template <auto Function, bool... Params>
using ImmediateBase =
        ColumnSimilarityMeasure<ImmediateBaseTypeTransformer<Function>,
                                BasicCalculator<BasicComparerCreator<Function>, Params...>>;
}  // namespace detail

template <auto Function, bool kSymmetric, bool kEqMax, bool... Params>
class ImmediateSimilarityMeasure
    : public detail::ImmediateBase<Function, kSymmetric, kEqMax, Params...> {
public:
    ImmediateSimilarityMeasure(
            std::string name, ColumnIdentifier left_column_identifier,
            ColumnIdentifier right_column_identifier, model::md::DecisionBoundary min_sim,
            std::size_t size_limit = 0,
            detail::ImmediateBaseTypeTransformer<Function>::TransformFunctionsOption funcs = {})
        : detail::ImmediateBase<Function, kSymmetric, kEqMax, Params...>(
                  kSymmetric && kEqMax, std::move(name), std::move(left_column_identifier),
                  std::move(right_column_identifier), {std::move(funcs)},
                  {min_sim, ccv_id_pickers::IndexUniform{size_limit}}) {};
};

template <auto Function>
using NormalImmediateSimilarityMeasure = ImmediateSimilarityMeasure<Function, true, true, true>;

}  // namespace algos::hymd::preprocessing::similarity_measure
