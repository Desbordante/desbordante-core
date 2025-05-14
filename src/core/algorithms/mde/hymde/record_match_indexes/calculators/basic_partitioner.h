#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <variant>

#include "algorithms/mde/hymde/record_match_indexes/calculators/partitioning_values_holder.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/value_index_map_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "config/exceptions.h"
#include "util/desbordante_assume.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename Ret1, typename Ret2>
class BasicPartitionCalculator {
public:
    using PartitioningFunctionLeft =
            std::unique_ptr<partitioning_functions::PartitioningFunction<Ret1>>;
    using PartitioningFunctionRight =
            std::unique_ptr<partitioning_functions::PartitioningFunction<Ret2>>;
    using PartitioningFunctionPair = std::pair<PartitioningFunctionLeft, PartitioningFunctionRight>;
    using PartitioningFunctionsOption =
            std::conditional_t<std::is_same_v<Ret1, Ret2>,
                               std::variant<PartitioningFunctionLeft, PartitioningFunctionPair>,
                               PartitioningFunctionPair>;

private:
    using Values = records::DictionaryCompressed::Values;
    using CompressedRecord = records::DictionaryCompressed::CompressedRecord;
    using Table = records::DictionaryCompressed::Table;
    using Adder = PartitionIndex::Adder;

    PartitioningFunctionsOption funcs_;
    records::DictionaryCompressed const& records_;

    template <typename R>
    bool Empty(std::unique_ptr<partitioning_functions::PartitioningFunction<R>> const& func) const {
        return func == nullptr;
    }

    template <typename T>
    auto Partition(Table const& table, partitioning_functions::PartitioningFunction<T> const& func,
                   Adder& adder, auto&&... other_adder) const {
        static_assert(sizeof...(other_adder) <= 1);
        using PartFunc = partitioning_functions::PartitioningFunction<T>;
        using R = std::invoke_result_t<std::remove_cvref_t<decltype(&PartFunc::GetValue)>, PartFunc,
                                       CompressedRecord const&>;

        auto add_old = [&](PartitionValueId value_id) {
            adder.AddToCluster(value_id);
            (other_adder.AddToCluster(value_id), ...);
        };
        auto add_new = [&](PartitionValueId value_id) {
            adder.AddToNewCluster(value_id);
            (other_adder.AddToNewCluster(value_id), ...);
        };

        ValueIndexMapBuilder<R> value_map_builder;
        for (CompressedRecord const& rec : table) {
            value_map_builder.AddValue(func.GetValue(rec), add_old, add_new);
        }
        return value_map_builder.TakeValues();
    }

    ParitioningValuesHolder<Ret1, Ret1> CreateSingle(auto&& func, Adder& left_adder,
                                                     Adder& right_adder) const {
        return Partition(records_.GetLeftTable(), func, left_adder, right_adder);
    }

    ParitioningValuesHolder<Ret1, Ret2> CreatePair(auto&& func1, Adder& left_adder, auto&& func2,
                                                   Adder& right_adder) const {
        return {Partition(records_.GetLeftTable(), func1, left_adder),
                Partition(records_.GetRightTable(), func2, right_adder)};
    }

public:
    BasicPartitionCalculator(PartitioningFunctionsOption funcs,
                             records::DictionaryCompressed const& records)
        : funcs_(std::move(funcs)), records_(records) {
        if (auto* function_ptr = std::get_if<PartitioningFunctionLeft>(&funcs_)) {
            if (Empty(*function_ptr)) throw config::ConfigurationError("Invalid function!");
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(funcs_));
            auto const& [left, right] = std::get<PartitioningFunctionPair>(funcs_);
            if (Empty(left) || Empty(right)) throw config::ConfigurationError("Invalid function!");
        }
    }

    ParitioningValuesHolder<Ret1, Ret2> Partition(Adder& left_adder, Adder& right_adder) const {
        if (auto* pair_ptr = std::get_if<PartitioningFunctionPair>(&funcs_)) {
            auto&& [l_func_ptr, r_func_ptr] = *pair_ptr;
            return CreatePair(*l_func_ptr, left_adder, *r_func_ptr, right_adder);
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionLeft>(funcs_));
            auto&& func_ptr = std::get<PartitioningFunctionLeft>(funcs_);
            if (records_.HoldsOneTable()) {
                return CreateSingle(*func_ptr, left_adder, right_adder);
            } else {
                return CreatePair(*func_ptr, left_adder, *func_ptr, right_adder);
            }
        }
    }

    std::string GetLeftName() const {
        if (auto* function_ptr = std::get_if<PartitioningFunctionLeft>(&funcs_)) {
            return (*function_ptr)->ToString();
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(funcs_));
            auto const& [left, right] = std::get<PartitioningFunctionPair>(funcs_);
            return left->ToString();
        }
    }

    std::string GetRightName() const {
        if (auto* function_ptr = std::get_if<PartitioningFunctionLeft>(&funcs_)) {
            return (*function_ptr)->ToString();
        } else {
            DESBORDANTE_ASSUME(std::holds_alternative<PartitioningFunctionPair>(funcs_));
            auto const& [left, right] = std::get<PartitioningFunctionPair>(funcs_);
            return right->ToString();
        }
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
