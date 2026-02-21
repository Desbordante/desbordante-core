#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/regex.hpp>

#include "core/algorithms/dd/dd.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/builtin.h"
#include "core/util/logger.h"

namespace algos::dd {

std::vector<DifferentialFunction> DifferentialFunctionBuilder::GetThresholds(
        model::TypedColumnData const& dif_column, model::ColumnIndex column_index) const {
    std::vector<DifferentialFunction> dfs;
    std::size_t dif_num_rows = dif_column.GetNumRows();

    auto pair_compare = [](model::DFConstraint const& first_pair,
                           model::DFConstraint const& second_pair) {
        return first_pair.LongerThan(second_pair);
    };

    std::set<model::DFConstraint, decltype(pair_compare)> limits(pair_compare);

    // accepts a string in the following format: [a;b], where a and b are double type values
    boost::regex df_regex(R"(\[(.*)\;(.*)\]$)");
    boost::regex double_regex(
            R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
            R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
            R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)");

    for (std::size_t row_index = 0; row_index < dif_num_rows; row_index++) {
        model::TypeId type_id = dif_column.GetValueTypeId(row_index);
        if (type_id == model::TypeId::kString) {
            std::string df_str = dif_column.GetDataAsString(row_index);
            boost::smatch matches;
            if (boost::regex_match(df_str, matches, df_regex)) {
                if (boost::regex_match(matches[1].str(), double_regex) &&
                    boost::regex_match(matches[2].str(), double_regex)) {
                    double const lower_limit =
                            model::TypeConverter<double>::kConvert(matches[1].str());
                    double const upper_limit =
                            model::TypeConverter<double>::kConvert(matches[2].str());

                    model::DFConstraint parsed_limits{lower_limit, upper_limit};

                    if (parsed_limits.IsValid()) {
                        auto intersect = parsed_limits.IntersectWith(min_max_dif_[column_index]);
                        if (intersect && *intersect != min_max_dif_[column_index]) {
                            limits.insert(*intersect);
                        }
                    }
                }
            }
        }
    }

    Column const* column = typed_relation_->GetColumnData(column_index).GetColumn();
    dfs.reserve(limits.size());
    std::ranges::transform(limits, std::back_inserter(dfs), [column](auto const& constraint) {
        return DifferentialFunction{constraint, column};
    });

    return dfs;
}

std::vector<std::size_t> DifferentialFunctionBuilder::SampleRows(std::size_t row_limit) const {
    std::random_device r;
    std::mt19937 rng(r());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, num_rows_ - 1);

    std::vector<std::size_t> row_nums;
    row_nums.reserve(row_limit);
    for (std::size_t i = 0; i != row_limit;) {
        std::size_t num = dist(rng);
        if (std::ranges::find(row_nums, num) == row_nums.end()) {
            row_nums.push_back(num);
            ++i;
        }
    }

    return row_nums;
}

std::pair<std::vector<double>, std::vector<double>> DifferentialFunctionBuilder::SampleThresholds(
        model::ColumnIndex const column_index, std::vector<std::size_t> const& row_nums,
        std::size_t row_limit, std::size_t threshold_num, double freq_boundary,
        double index_boundary) const {
    std::unordered_map<double, unsigned> diff_freq_map;

    std::set<double> thresholds_set;
    for (std::size_t i = 0; i != row_limit - 1; ++i) {
        for (std::size_t j = i + 1; j != row_limit; ++j) {
            double dif = distance_calculator_->CalculateDistance(column_index,
                                                                 {row_nums[i], row_nums[j]});
            thresholds_set.insert(dif);
            auto&& [it, is_value_new] = diff_freq_map.try_emplace(dif, 0);
            ++it->second;
        }
    }
    std::vector<double> thresholds;
    thresholds.insert(thresholds.end(), thresholds_set.begin(), thresholds_set.end());
    std::vector<unsigned> frequencies;
    frequencies.reserve(thresholds.size());
    std::ranges::transform(thresholds, std::back_inserter(frequencies),
                           [&diff_freq_map](auto threshold) { return diff_freq_map[threshold]; });

    std::size_t unique_thresholds_num = diff_freq_map.size();

    std::vector<double> less_thresholds;
    std::vector<double> greater_thresholds;

    if (unique_thresholds_num <= 1) {
        less_thresholds.push_back(0);
        if (!model::IsEqual(thresholds[0], 0.0)) {
            less_thresholds.push_back(thresholds[0]);
        }
        greater_thresholds = less_thresholds;
        return {std::move(less_thresholds), std::move(greater_thresholds)};
    }

    if (unique_thresholds_num < threshold_num) {
        less_thresholds.push_back(0);
        if (!model::IsEqual(thresholds[0], 0.0)) {
            less_thresholds.push_back(thresholds[0]);
        }
        for (std::size_t i = 0; i != unique_thresholds_num / 2; ++i) {
            less_thresholds.push_back(thresholds[i]);
        }
        for (std::size_t i = unique_thresholds_num / 2; i != unique_thresholds_num; ++i) {
            greater_thresholds.push_back(thresholds[i]);
        }
        return {std::move(less_thresholds), std::move(greater_thresholds)};
    }

    unsigned long total_freq_sum = 0;
    for (auto const& [threshold, freq] : diff_freq_map) {
        total_freq_sum += freq;
    }

    std::size_t last_less_threshold_index = 0;
    unsigned long cur_freq_sum = 0;
    for (std::size_t i = 0; i != thresholds.size(); ++i) {
        cur_freq_sum += diff_freq_map.at(thresholds[i]);
        if (total_freq_sum * freq_boundary <= cur_freq_sum ||
            i >= unique_thresholds_num * index_boundary) {
            cur_freq_sum = 0;
            last_less_threshold_index = i;
            break;
        }
    }
    std::size_t first_right_threshold_index = 0;
    for (std::size_t i = thresholds.size() - 1; i != 0; --i) {
        cur_freq_sum += diff_freq_map.at(thresholds[i]);
        if (total_freq_sum * freq_boundary <= cur_freq_sum) {
            first_right_threshold_index = i;
            break;
        }
    }

    less_thresholds.push_back(0);
    double max_score = -1;
    std::size_t temp_index = 0;
    std::size_t index = 0;
    for (std::size_t count = 1; count <= threshold_num / 2; ++count) {
        for (std::size_t i = temp_index + 1; i <= last_less_threshold_index; ++i) {
            double interval_length = thresholds[i] - thresholds[temp_index];
            unsigned long cur_freq_sum = 0;
            for (std::size_t j = temp_index; j <= i; ++j) {
                cur_freq_sum += frequencies[j];
            }
            double score = cur_freq_sum / interval_length;
            if (score > max_score) {
                max_score = score;
                index = i;
            }
        }
        less_thresholds.push_back(thresholds[index]);
        temp_index = index;
        max_score = -1;
    }

    temp_index = first_right_threshold_index;
    for (std::size_t count = threshold_num / 2 + 1; count < threshold_num; ++count) {
        for (std::size_t i = temp_index; i < unique_thresholds_num - 1; ++i) {
            double interval_length = thresholds[unique_thresholds_num - 1] - thresholds[i];
            unsigned long cur_freq_sum = 0;
            for (std::size_t j = i + 1; j < unique_thresholds_num; ++j) {
                cur_freq_sum += frequencies[j];
            }
            double score = cur_freq_sum / interval_length;
            if (score > max_score) {
                max_score = score;
                index = i;
            }
        }
        greater_thresholds.push_back(thresholds[index]);
        temp_index = index + 1;
        max_score = -1;
    }

    std::set<double> less_thresholds_set(less_thresholds.begin(), less_thresholds.end());
    std::vector<double> unique_less_thresholds(less_thresholds_set.begin(),
                                               less_thresholds_set.end());

    std::set<double> greater_thresholds_set(greater_thresholds.begin(), greater_thresholds.end());
    std::vector<double> unique_greater_thresholds(greater_thresholds_set.begin(),
                                                  greater_thresholds_set.end());

    return {std::move(unique_less_thresholds), std::move(unique_greater_thresholds)};
}

void DifferentialFunctionBuilder::AddThresholds(std::vector<double> const& less_thresholds,
                                                std::vector<double> const& greater_thresholds,
                                                model::ColumnIndex const column_index) {
    auto pair_compare = [](model::DFConstraint const& first_pair,
                           model::DFConstraint const& second_pair) {
        return first_pair.LongerThan(second_pair);
    };

    std::set<model::DFConstraint, decltype(pair_compare)> limits(pair_compare);

    Column const* column = typed_relation_->GetColumnData(column_index).GetColumn();
    for (auto threshold_it = less_thresholds.rbegin(); threshold_it != less_thresholds.rend();
         ++threshold_it) {
        auto intersect =
                model::DFConstraint{0UL, *threshold_it}.IntersectWith(min_max_dif_[column_index]);
        if (intersect && *intersect != min_max_dif_[column_index]) {
            limits.insert(*intersect);
        }
    }
    for (auto threshold_it = greater_thresholds.begin(); threshold_it != greater_thresholds.end();
         ++threshold_it) {
        auto intersect = model::DFConstraint{*threshold_it, std::numeric_limits<double>::infinity()}
                                 .IntersectWith(min_max_dif_[column_index]);
        if (intersect && *intersect != min_max_dif_[column_index]) {
            limits.insert(*intersect);
        }
    }

    if (!limits.empty()) {
        differential_functions_.emplace_back();
        differential_functions_.back().reserve(limits.size());
        std::ranges::transform(limits, std::back_inserter(differential_functions_.back()),
                               [column](auto const& constraint) {
                                   return DifferentialFunction{constraint, column};
                               });
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
        std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation) {
    LOG_DEBUG("Number of columns: {}", num_columns_);
    differential_functions_.reserve(num_columns_);
    dif_func_nums_.reserve(num_columns_ + 1);
    dif_func_nums_.push_back(0);

    if (difference_typed_relation) {
        for (model::ColumnIndex column_index = 0; column_index != num_columns_; ++column_index) {
            auto dif_funcs = GetThresholds(difference_typed_relation->GetColumnData(column_index),
                                           column_index);
            if (!dif_funcs.empty()) {
                differential_functions_.push_back(std::move(dif_funcs));
                dif_func_nums_.push_back(dif_func_nums_.back() +
                                         differential_functions_.back().size());
            }
        }
    } else {
        std::size_t const row_limit = std::min(200UL, num_rows_ / 5UL);
        std::vector<std::size_t> row_nums = SampleRows(row_limit);
        for (model::ColumnIndex column_index = 0; column_index != num_columns_; ++column_index) {
            auto const [less_thresholds, greater_thresholds] =
                    SampleThresholds(column_index, row_nums, row_limit, 5UL, 0.3,
                                     0.75); /* all magic constants for threshold sampling are
                                               taken from the original implementation */
            AddThresholds(less_thresholds, greater_thresholds, column_index);
        }
    }
    CalculateThresholdZones();
}

}  // namespace algos::dd
