#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "algorithms/mde/decision_boundaries/unsigned_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/max_size_inspector.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder_supplier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/value_calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer.h"
#include "algorithms/mde/hymde/utility/make_unique_for_overwrite.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace levenshtein_distance {
using PartitioningValue = std::string;
using ComparisonResult = std::size_t;

class LevenshteinComparerCreator {
    struct Comparer {
        std::unique_ptr<unsigned[]> buf;
        unsigned* r_buf;
        ComparisonResult cutoff_;

        ComparisonResult operator()(PartitioningValue const& l, PartitioningValue const& r);
    };

    ComparisonResult cutoff_;
    std::size_t const buf_len_;

public:
    LevenshteinComparerCreator(ComparisonResult cutoff, std::size_t max_size)
        : cutoff_(cutoff), buf_len_(max_size) {}

    Comparer operator()() const {
        // TODO: replace with std::make_unique_for_overwrite when GCC in CI is upgraded
        auto buf = utility::MakeUniqueForOverwrite<unsigned[]>(buf_len_ * 2);
        auto* buf_ptr = buf.get();
        return {std::move(buf), buf_ptr + buf_len_, cutoff_};
    }
};

class LevenshteinComparerCreatorSupplier {
    ComparisonResult cutoff_;

public:
    using ComparerCreator = LevenshteinComparerCreator;

    LevenshteinComparerCreatorSupplier(ComparisonResult cutoff) : cutoff_(cutoff) {}

    ComparerCreator operator()(auto&& part_info) const {
        return {cutoff_, part_info.inspector.GetMaxSize()};
    }
};

using BuilderSupplier = SameValueTypeBuilderSupplier<PartitioningValue, SingleMaxSizeInspector,
                                                     PairMaxSizeInspector>;

using EqValue = utility::CompileTimeOptionalLike<ComparisonResult{0}>;
using Symmetric = utility::CompileTimeValue<true>;

using LevenshteinSimilarityBase = CalculatorImpl<
        BuilderSupplier,
        ValueCalculator<LevenshteinComparerCreatorSupplier,
                        model::mde::decision_boundaries::UnsignedInteger, EqValue, Symmetric>>;
}  // namespace levenshtein_distance

class LevenshteinDistance : public levenshtein_distance::LevenshteinSimilarityBase {
    using Base = levenshtein_distance::LevenshteinSimilarityBase;
    using CompCreatorSupplier = levenshtein_distance::LevenshteinComparerCreatorSupplier;
    using PartitioningValue = levenshtein_distance::PartitioningValue;

public:
    using PartitioningValueLeft = PartitioningValue;
    using PartitioningValueRight = PartitioningValue;
    using ComparisonResult = levenshtein_distance::ComparisonResult;

    using OrderPtr = std::shared_ptr<orders::UnsignedInteger>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;
    using PartitioningFunctionsOption = Base::PartitioningFunctionsOption;

    using Creator = StandardCalculatorCreator<LevenshteinDistance>;

public:
    LevenshteinDistance(records::DictionaryCompressed const& records,
                        PartitioningFunctionsOption partitioning_functions, OrderPtr order,
                        SelectorPtr selector, ComparisonResult cutoff)
        : Base("levenshtein_distance", std::move(partitioning_functions), records, {},
               {levenshtein_distance::EqValue{}, levenshtein_distance::Symmetric{},
                utility::CompileTimeValue<true>{}, CompCreatorSupplier{cutoff}, std::move(order),
                std::move(selector)}) {}
};
}  // namespace algos::hymde::record_match_indexes::calculators
