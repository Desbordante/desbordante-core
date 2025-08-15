#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename PartitionBuilderSupplier, typename ValueCalculator>
class CalculatorImpl : public Calculator {
    using LeftElement = PartitionBuilderSupplier::LeftElement;
    using RightElement = PartitionBuilderSupplier::RightElement;

    using PartitioningFunctionLeft =
            std::unique_ptr<partitioning_functions::PartitioningFunction<LeftElement>>;
    using PartitioningFunctionRight =
            std::unique_ptr<partitioning_functions::PartitioningFunction<RightElement>>;
    using PartitioningFunctionPair = std::pair<PartitioningFunctionLeft, PartitioningFunctionRight>;

    constexpr static bool kPartValuesAreSame = std::is_same_v<LeftElement, RightElement>;

public:
    using PartitioningFunctionsOption =
            std::conditional_t<kPartValuesAreSame,
                               std::variant<PartitioningFunctionLeft, PartitioningFunctionPair>,
                               PartitioningFunctionPair>;

private:
    using Adder = PartitionIndex::PartitionBuilder;

    using CompressedRecord = records::DictionaryCompressed::CompressedRecord;

    void AddAllRecords(auto&& builder, auto&& func, auto&& table, auto method) const {
        for (CompressedRecord const& rec : table) {
            (builder.*method)(func.GetValue(rec));
        }
    }

    ComponentHandlingInfo CalcSingle(util::WorkerThreadPool* pool_ptr, auto&& func,
                                     Adder& left_adder, Adder& right_adder) const {
        auto single_builder = builder_supplier_.GetSingle(left_adder, right_adder);
        AddAllRecords(single_builder, func, records_.GetLeftTable(),
                      &decltype(single_builder)::AddValue);
        return value_calculator_.CalculateSingle(single_builder.Build(),
                                                 single_builder.GetRightPli(), pool_ptr);
    }

    ComponentHandlingInfo CalcPair(util::WorkerThreadPool* pool_ptr, auto&& func1,
                                   Adder& left_adder, auto&& func2, Adder& right_adder) const {
        auto pair_builder = builder_supplier_.GetPair(left_adder, right_adder);
        AddAllRecords(pair_builder, func1, records_.GetLeftTable(),
                      &decltype(pair_builder)::AddLeftValue);
        AddAllRecords(pair_builder, func2, records_.GetRightTable(),
                      &decltype(pair_builder)::AddRightValue);
        return value_calculator_.CalculatePair(pair_builder.Build(), pair_builder.GetRightPli(),
                                               pool_ptr);
    }

    static std::string GetLeftName(PartitioningFunctionsOption const& part_funcs) {
        if (auto* function_ptr = std::get_if<PartitioningFunctionLeft>(&part_funcs)) {
            return (*function_ptr)->ToString();
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(part_funcs));
            auto const& [left, right] = std::get<PartitioningFunctionPair>(part_funcs);
            return left->ToString();
        }
    }

    static std::string GetRightName(PartitioningFunctionsOption const& part_funcs) {
        if (auto* function_ptr = std::get_if<PartitioningFunctionLeft>(&part_funcs)) {
            return (*function_ptr)->ToString();
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(part_funcs));
            auto const& [left, right] = std::get<PartitioningFunctionPair>(part_funcs);
            return right->ToString();
        }
    }

    [[no_unique_address]] PartitionBuilderSupplier builder_supplier_;
    [[no_unique_address]] ValueCalculator value_calculator_;
    PartitioningFunctionsOption part_funcs_;
    records::DictionaryCompressed const& records_;

public:
    CalculatorImpl(std::string comparison_function, PartitioningFunctionsOption part_funcs,
                   records::DictionaryCompressed const& records,
                   PartitionBuilderSupplier builder_supplier, ValueCalculator value_calculator)
        : Calculator({GetLeftName(part_funcs), GetRightName(part_funcs),
                      std::move(comparison_function), value_calculator.GetOrderName()}),
          builder_supplier_(std::move(builder_supplier)),
          value_calculator_(std::move(value_calculator)),
          part_funcs_(std::move(part_funcs)),
          records_(records) {}

    ComponentHandlingInfo Calculate(util::WorkerThreadPool* pool_ptr,
                                    PartitionIndex::PartitionBuilder&& left_adder,
                                    PartitionIndex::PartitionBuilder&& right_adder) const final {
        if constexpr (kPartValuesAreSame) {
            if (auto* func_ptr = std::get_if<PartitioningFunctionLeft>(&part_funcs_)) {
                if (records_.HoldsOneTable()) {
                    return CalcSingle(pool_ptr, **func_ptr, left_adder, right_adder);
                } else {
                    return CalcPair(pool_ptr, **func_ptr, left_adder, **func_ptr, right_adder);
                }
            } else {
                DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(part_funcs_));
                auto&& [l_func_ptr, r_func_ptr] = std::get<PartitioningFunctionPair>(part_funcs_);
                return CalcPair(pool_ptr, *l_func_ptr, left_adder, *r_func_ptr, right_adder);
            }
        } else {
            auto&& [l_func_ptr, r_func_ptr] = part_funcs_;
            return CalcPair(pool_ptr, *l_func_ptr, left_adder, *r_func_ptr, right_adder);
        }
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
