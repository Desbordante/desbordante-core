#pragma once

#include <string>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"

namespace algos::hymde::record_match_indexes::calculators {
// if only one function is given and the table is the same, we can assume that calling it over the
// table will output the same results
template <typename PartitionIndexCalculator, typename ValueIndexCalculator>
class CalculatorImpl : public Calculator {
    PartitionIndexCalculator partition_calculator_;
    ValueIndexCalculator value_calculator_;

public:
    [[nodiscard]] ComponentHandlingInfo Calculate(util::WorkerThreadPool* pool_ptr,
                                                  PartitionIndex::Adder&& left_adder,
                                                  PartitionIndex::Adder&& right_adder) const final {
        auto partitioning_info = partition_calculator_.Partition(left_adder, right_adder);
        PartitionIndex::PositionListIndex const& right_pli = right_adder.GetCurrentPli();
        return value_calculator_.Calculate(std::move(partitioning_info), right_pli, pool_ptr);
    }

    CalculatorImpl(std::string comparison_function, PartitionIndexCalculator partition_calculator,
                   ValueIndexCalculator value_calculator)
        : Calculator({partition_calculator.GetLeftName(), partition_calculator.GetRightName(),
                      std::move(comparison_function), value_calculator.GetOrderName()}),
          partition_calculator_(std::move(partition_calculator)),
          value_calculator_(std::move(value_calculator)) {}
};
}  // namespace algos::hymde::record_match_indexes::calculators
