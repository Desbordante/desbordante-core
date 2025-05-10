#pragma once

#include <memory>
#include <string>

#include "algorithms/mde/decision_boundaries/similarity.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/basic_partitioner.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/basic_value_calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
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

    // TODO: make this part of partitioner.
    static std::size_t GetLargestStringSize(std::vector<PartitioningValue> const& elements);

public:
    LevenshteinComparerCreator(ComparisonResult sim_cutoff,
                               std::vector<PartitioningValue> const* left_elements)
        : sim_cutoff_(sim_cutoff), buf_len_(GetLargestStringSize(*left_elements) + 1) {}

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
    LevenshteinComparerCreatorSupplier(ComparisonResult sim_cutoff) : sim_cutoff_(sim_cutoff) {}

    LevenshteinComparerCreator operator()(std::vector<PartitioningValue> const* left_elements,
                                          std::vector<PartitioningValue> const*) const {
        return {sim_cutoff_, left_elements};
    }
};

// TODO: record max string size in partitioner
using LevenshteinSimilarityPartitioner =
        BasicPartitionCalculator<PartitioningValue, PartitioningValue>;

using LevenshteinSimilarityBase =
        CalculatorImpl<LevenshteinSimilarityPartitioner,
                       BasicValueCalculator<LevenshteinComparerCreatorSupplier,
                                            model::mde::decision_boundaries::Similarity, true>>;
}  // namespace levenshtein_similarity

class LevenshteinSimilarity : public levenshtein_similarity::LevenshteinSimilarityBase {
    using Base = levenshtein_similarity::LevenshteinSimilarityBase;
    using CompCreatorSupplier = levenshtein_similarity::LevenshteinComparerCreatorSupplier;
    using Partitioner = levenshtein_similarity::LevenshteinSimilarityPartitioner;
    using PartitioningValue = levenshtein_similarity::PartitioningValue;

public:
    using PartitioningValueLeft = PartitioningValue;
    using PartitioningValueRight = PartitioningValue;
    using ComparisonResult = levenshtein_similarity::ComparisonResult;

    using OrderPtr = std::shared_ptr<orders::TotalOrder<ComparisonResult>>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;
    using PartitioningFunctionsOption = Partitioner::PartitioningFunctionsOption;

    using Creator = StandardCalculatorCreator<LevenshteinSimilarity>;

public:
    LevenshteinSimilarity(records::DictionaryCompressed const& records,
                          PartitioningFunctionsOption partitioning_functions, OrderPtr order,
                          SelectorPtr selector, ComparisonResult similarity_cutoff)
        : Base("levenshtein_similarity", Partitioner{std::move(partitioning_functions), records},
               {CompCreatorSupplier{similarity_cutoff}, std::move(order), std::move(selector),
                1.0}) {}
};
}  // namespace algos::hymde::record_match_indexes::calculators
