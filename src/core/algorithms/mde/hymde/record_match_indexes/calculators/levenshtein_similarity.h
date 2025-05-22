#pragma once

#include <memory>
#include <optional>
#include <string>

#include "algorithms/mde/decision_boundaries/float.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/max_size_inspector.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder_supplier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/value_calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity.h"
#include "algorithms/mde/hymde/utility/make_unique_for_overwrite.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace levenshtein_similarity {
using PartitioningValue = std::string;
using ComparisonResult = double;

class LevenshteinComparerCreator {
    struct Comparer {
        std::unique_ptr<unsigned[]> buf;
        unsigned* r_buf;
        ComparisonResult sim_cutoff_;

        ComparisonResult operator()(PartitioningValue const& l, PartitioningValue const& r);
    };

    ComparisonResult sim_cutoff_;
    std::size_t const buf_len_;

public:
    LevenshteinComparerCreator(ComparisonResult sim_cutoff, std::size_t max_size)
        : sim_cutoff_(sim_cutoff), buf_len_(max_size) {}

    Comparer operator()() const {
        // TODO: replace with std::make_unique_for_overwrite when GCC in CI is upgraded
        auto buf = utility::MakeUniqueForOverwrite<unsigned[]>(buf_len_ * 2);
        auto* buf_ptr = buf.get();
        return {std::move(buf), buf_ptr + buf_len_, sim_cutoff_};
    }
};

class LevenshteinComparerCreatorSupplier {
    ComparisonResult sim_cutoff_;

public:
    using ComparerCreator = LevenshteinComparerCreator;

    LevenshteinComparerCreatorSupplier(ComparisonResult sim_cutoff) : sim_cutoff_(sim_cutoff) {}

    ComparerCreator operator()(auto&& part_info) const {
        return {sim_cutoff_, part_info.inspector.GetMaxSize()};
    }
};

using BuilderSupplier = SameValueTypeBuilderSupplier<PartitioningValue, SingleMaxSizeInspector,
                                                     PairMaxSizeInspector>;

using EqValue = utility::CompileTimeOptionalLike<1.0>;
using Symmetric = utility::CompileTimeValue<true>;

using LevenshteinSimilarityBase =
        CalculatorImpl<BuilderSupplier,
                       ValueCalculator<LevenshteinComparerCreatorSupplier,
                                       model::mde::decision_boundaries::Float, EqValue, Symmetric>>;
}  // namespace levenshtein_similarity

class LevenshteinSimilarity : public levenshtein_similarity::LevenshteinSimilarityBase {
    using Base = levenshtein_similarity::LevenshteinSimilarityBase;
    using CompCreatorSupplier = levenshtein_similarity::LevenshteinComparerCreatorSupplier;
    using PartitioningValue = levenshtein_similarity::PartitioningValue;

public:
    using PartitioningValueLeft = PartitioningValue;
    using PartitioningValueRight = PartitioningValue;
    using ComparisonResult = levenshtein_similarity::ComparisonResult;

    using OrderPtr = std::shared_ptr<orders::Similarity>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;
    using PartitioningFunctionsOption = Base::PartitioningFunctionsOption;

    using Creator = StandardCalculatorCreator<LevenshteinSimilarity>;

public:
    LevenshteinSimilarity(records::DictionaryCompressed const& records,
                          PartitioningFunctionsOption partitioning_functions, OrderPtr order,
                          SelectorPtr selector, ComparisonResult similarity_cutoff)
        : Base("levenshtein_similarity", std::move(partitioning_functions), records, {},
               {levenshtein_similarity::EqValue{}, levenshtein_similarity::Symmetric{},
                utility::CompileTimeValue<true>{}, CompCreatorSupplier{similarity_cutoff},
                std::move(order), std::move(selector)}) {}
};
}  // namespace algos::hymde::record_match_indexes::calculators
