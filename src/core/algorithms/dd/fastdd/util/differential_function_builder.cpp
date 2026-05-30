#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/model/table/column.h"
#include "core/util/logger.h"

namespace algos::dd {

void DifferentialFunctionBuilder::AddThresholds(std::vector<model::DFConstraint> const& thresholds,
                                                model::ColumnIndex const column_index,
                                                std::size_t const offset,
                                                model::DFConstraint const& min_max_dif) {
    auto pair_compare = [](std::pair<model::DFConstraint, std::size_t> const& first_pair,
                           std::pair<model::DFConstraint, std::size_t> const& second_pair) {
        return first_pair.first.LongerThan(second_pair.first);
    };

    std::set<std::pair<model::DFConstraint, std::size_t>, decltype(pair_compare)> limits(
            pair_compare);

    Column const* column = typed_relation_->GetColumnData(column_index).GetColumn();

    for (std::size_t threshold_index = 0; threshold_index != thresholds.size(); ++threshold_index) {
        auto intersect = thresholds[threshold_index].IntersectWith(min_max_dif);
        if (intersect && *intersect != min_max_dif) {
            limits.emplace(*intersect, threshold_index + offset);
        }
    }

    if (!limits.empty()) {
        differential_functions_.emplace_back();
        differential_functions_.back().reserve(limits.size());
        std::ranges::transform(limits, std::back_inserter(differential_functions_.back()),
                               [column](auto const& constraint_pair) {
                                   return DifferentialFunction{constraint_pair.first, column};
                               });
        std::size_t cur_offset = 0;
        for (auto const& [_, old_index] : limits) {
            old_index_to_new_index_[old_index] = dif_func_nums_.back() + cur_offset;
            ++cur_offset;
        }
        dif_func_nums_.push_back(dif_func_nums_.back() + differential_functions_.back().size());
        LOG_DEBUG("Column: {}", column_index);
        for (auto const& dif_func : differential_functions_.back()) {
            LOG_DEBUG(dif_func.ToString());
        }
    }
}

void DifferentialFunctionBuilder::CalculateThresholdZones() {
    std::size_t const all_dif_funcs_size = differential_functions_.size();
    zone_to_bitset_.reserve(all_dif_funcs_size);
    thresholds_.reserve(all_dif_funcs_size);
    threshold_zones_.reserve(all_dif_funcs_size);
    std::size_t const bitset_size = dif_func_nums_[dif_func_nums_.size() - 1];
    for (model::ColumnIndex column_index = 0; column_index != all_dif_funcs_size; ++column_index) {
        zone_to_bitset_.emplace_back();
        std::size_t const dif_funcs_size = differential_functions_[column_index].size();
        thresholds_.emplace_back();
        auto& cur_thresholds = thresholds_[column_index];
        cur_thresholds.reserve(2 * dif_funcs_size);
        for (std::size_t dif_func_index = 0; dif_func_index != dif_funcs_size; ++dif_func_index) {
            auto constraint = differential_functions_[column_index][dif_func_index].GetConstraint();
            cur_thresholds.emplace_back(constraint.lower_bound, false, dif_func_index);
            cur_thresholds.emplace_back(constraint.upper_bound, true, dif_func_index);
        }
        std::ranges::sort(thresholds_[column_index]);
        threshold_zones_.emplace_back();
        threshold_zones_[column_index].reserve(thresholds_[column_index].size() + 1);
        std::unordered_map<boost::dynamic_bitset<>, std::size_t> zone_index_map;
        std::size_t const cur_index = dif_func_nums_[column_index];
        boost::dynamic_bitset<> cur_bitset(bitset_size);
        std::size_t cur_zone_index = 0;
        zone_index_map.emplace(cur_bitset, cur_zone_index);
        zone_to_bitset_[column_index].push_back(cur_bitset);
        threshold_zones_[column_index].emplace_back(cur_zone_index);
        for (std::size_t index = 0; index != cur_thresholds.size(); ++index) {
            cur_bitset[cur_index + cur_thresholds[index].dif_func_index] =
                    !cur_thresholds[index].is_upper_bound;
            auto [it, is_new] = zone_index_map.try_emplace(cur_bitset, cur_zone_index + 1);
            if (is_new) {
                zone_to_bitset_[column_index].push_back(cur_bitset);
                ++cur_zone_index;
            }
            threshold_zones_[column_index].emplace_back(it->second);
        }
    }
}

void DifferentialFunctionBuilder::BuildDFList(
        std::vector<std::vector<model::DFConstraint>> const& thresholds,
        std::vector<model::DFConstraint> const& min_max_dif) {
    LOG_DEBUG("Number of columns: {}", num_columns_);
    differential_functions_.reserve(thresholds.size());
    dif_func_nums_.reserve(thresholds.size() + 1);
    dif_func_nums_.push_back(0);
    std::size_t const threshold_num =
            std::accumulate(thresholds.begin(), thresholds.end(), 0UL,
                            [](std::size_t res, auto const& col_thresholds) {
                                return res + col_thresholds.size();
                            });
    old_index_to_new_index_ = std::vector<std::size_t>(threshold_num, threshold_num + 1);

    std::size_t offset = 0;
    for (model::ColumnIndex column_index = 0; column_index != thresholds.size(); ++column_index) {
        AddThresholds(thresholds[column_index], column_index, offset,
                      min_max_dif.empty()
                              ? model::DFConstraint{0, std::numeric_limits<double>::max()}
                              : min_max_dif[column_index]);
        offset += thresholds[column_index].size();
    }
    CalculateThresholdZones();
}

void DifferentialFunctionBuilder::UpdateDFList(
        std::vector<model::DFConstraint> const& min_max_dif) {
    std::vector<std::vector<model::DFConstraint>> thresholds;
    thresholds.reserve(differential_functions_.size());
    for (std::size_t index = 0; index != differential_functions_.size(); ++index) {
        thresholds.emplace_back();
        thresholds[index].reserve(differential_functions_[index].size());
        std::ranges::transform(differential_functions_[index],
                               std::back_inserter(thresholds[index]),
                               [](auto const& dif_func) { return dif_func.GetConstraint(); });
    }

    differential_functions_.clear();
    dif_func_nums_.clear();
    thresholds_.clear();
    threshold_zones_.clear();
    zone_to_bitset_.clear();

    BuildDFList(thresholds, min_max_dif);
}

}  // namespace algos::dd
