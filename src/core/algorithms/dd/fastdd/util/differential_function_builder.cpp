#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"

#include <algorithm>
#include <cstddef>
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

std::pair<std::vector<double>, std::vector<double>> DifferentialFunctionBuilder::GetThresholds(
        model::TypedColumnData const& dif_column) const {
    std::size_t dif_num_rows = dif_column.GetNumRows();

    boost::regex df_regex(R"((>|<=) (.*)$)");
    boost::regex double_regex(
            R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
            R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
            R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)");

    std::vector<double> less_thresholds;
    std::vector<double> greater_thresholds;

    std::set<double> less_thresholds_set;
    std::set<double> greater_thresholds_set;
    less_thresholds_set.insert(0.0);  // for repeatability

    for (std::size_t row_index = 0; row_index < dif_num_rows; row_index++) {
        model::TypeId type_id = dif_column.GetValueTypeId(row_index);
        if (type_id == model::TypeId::kString) {
            std::string df_str = dif_column.GetDataAsString(row_index);
            boost::smatch matches;
            if (boost::regex_match(df_str, matches, df_regex)) {
                if (boost::regex_match(matches[2].str(), double_regex)) {
                    double const threshold =
                            model::TypeConverter<double>::kConvert(matches[2].str());
                    if (matches[1].str() == "<=") {
                        less_thresholds_set.insert(threshold);
                    } else {
                        greater_thresholds_set.insert(threshold);
                    }
                }
            }
        }
    }

    less_thresholds.insert(less_thresholds.end(), less_thresholds_set.begin(),
                           less_thresholds_set.end());
    greater_thresholds.insert(greater_thresholds.end(), greater_thresholds_set.begin(),
                              greater_thresholds_set.end());
    return {std::move(less_thresholds), std::move(greater_thresholds)};
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
    std::set<double> threshold_set;
    threshold_set.insert(less_thresholds.begin(), less_thresholds.end());
    threshold_set.insert(greater_thresholds.begin(), greater_thresholds.end());
    thresholds_[column_index].insert(thresholds_[column_index].end(), threshold_set.begin(),
                                     threshold_set.end());

    differential_functions_[column_index].reserve(less_thresholds.size() +
                                                  greater_thresholds.size());
    Column const* column = typed_relation_->GetColumnData(column_index).GetColumn();
    for (auto threshold_it = less_thresholds.rbegin(); threshold_it != less_thresholds.rend();
         ++threshold_it) {
        differential_functions_[column_index].push_back(df_provider_.GetDifferentialFunction(
                Operator::kLessOrEqual, column, *threshold_it));
    }
    for (auto threshold_it = greater_thresholds.begin(); threshold_it != greater_thresholds.end();
         ++threshold_it) {
        differential_functions_[column_index].push_back(
                df_provider_.GetDifferentialFunction(Operator::kGreater, column, *threshold_it));
    }
}

void DifferentialFunctionBuilder::BuildDFList(
        std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation) {
    LOG_DEBUG("{}", num_columns_);
    differential_functions_.reserve(num_columns_);
    dif_func_nums_.reserve(num_columns_ + 1);
    dif_func_nums_.push_back(0);

    auto print_column_thresholds = [](std::vector<double> const& less_thresholds,
                                      std::vector<double> const& greater_thresholds) {
        LOG_DEBUG("Less:");
        for (auto less_threshold : less_thresholds) {
            LOG_DEBUG("{}", less_threshold);
        }
        LOG_DEBUG("Greater:");
        for (auto greater_threshold : greater_thresholds) {
            LOG_DEBUG("{}", greater_threshold);
        }
    };

    if (difference_typed_relation) {
        for (model::ColumnIndex column_index = 0; column_index != num_columns_; ++column_index) {
            auto const [less_thresholds, greater_thresholds] =
                    GetThresholds(difference_typed_relation->GetColumnData(column_index));
            differential_functions_.emplace_back();
            AddThresholds(less_thresholds, greater_thresholds, column_index);
            print_column_thresholds(less_thresholds, greater_thresholds);
            dif_func_nums_.push_back(dif_func_nums_[column_index] +
                                     differential_functions_[column_index].size());
        }
    } else {
        std::size_t const row_limit = std::min(200UL, num_rows_ / 5UL);
        std::vector<std::size_t> row_nums = SampleRows(row_limit);
        for (model::ColumnIndex column_index = 0; column_index != num_columns_; ++column_index) {
            auto const [less_thresholds, greater_thresholds] =
                    SampleThresholds(column_index, row_nums, row_limit, 5UL, 0.3,
                                     0.75); /* all magic constants for threshold sampling are
                                               taken from the original implementation */
            differential_functions_.emplace_back();
            AddThresholds(less_thresholds, greater_thresholds, column_index);
            LOG_DEBUG("Column: {}", column_index);
            print_column_thresholds(less_thresholds, greater_thresholds);
            dif_func_nums_.push_back(dif_func_nums_[column_index] +
                                     differential_functions_[column_index].size());
        }
    }
}

std::vector<boost::dynamic_bitset<>> DifferentialFunctionBuilder::GetSatisfiedDFs(
        model::ColumnIndex const column_index) const {
    std::vector<boost::dynamic_bitset<>> bitsets;
    bitsets.reserve(thresholds_[column_index].size() + 1);
    std::size_t const bitset_size = dif_func_nums_[dif_func_nums_.size() - 1];
    for (auto const threshold : thresholds_[column_index]) {
        boost::dynamic_bitset<> cur_bitset(bitset_size);
        for (std::size_t i = 0; i != differential_functions_[column_index].size(); ++i) {
            DifferentialFunction const& dif_func = differential_functions_[column_index][i];
            std::size_t const index = dif_func_nums_[column_index] + i;
            if (dif_func.GetOperator() == Operator::kLessOrEqual &&
                model::LessOrEqual(threshold, dif_func.GetThreshold())) {
                cur_bitset.set(index);
            } else if (dif_func.GetOperator() == Operator::kGreater &&
                       model::Greater(threshold, dif_func.GetThreshold())) {
                cur_bitset.set(index);
            }
        }
        bitsets.push_back(std::move(cur_bitset));
    }
    boost::dynamic_bitset<> last_bitset(bitset_size);
    for (std::size_t i = 0; i != differential_functions_[column_index].size(); ++i) {
        DifferentialFunction const& dif_func = differential_functions_[column_index][i];
        std::size_t const index = dif_func_nums_[column_index] + i;
        if (dif_func.GetOperator() == Operator::kGreater) {
            last_bitset.set(index);
        }
    }
    bitsets.push_back(std::move(last_bitset));

    return bitsets;
}

}  // namespace algos::dd
