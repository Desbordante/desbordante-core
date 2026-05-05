#include "core/algorithms/dd/fastdd/util/min_max_dif_calculator.h"

#include <cstddef>
#include <limits>

#include "core/model/table/column_index.h"

namespace algos::dd {

MinMaxDifCalculator::MinMaxDifCalculator(std::shared_ptr<DistanceCalculator> distance_calculator,
                                         std::vector<PliShard> const& pli_shards)
    : distance_calculator_(distance_calculator),
      min_max_dif_(pli_shards[0].Plis().size(), {std::numeric_limits<double>::max(), 0}) {
    for (std::size_t i = 0; i != pli_shards.size(); ++i) {
        CalculateSingleMinMaxDif(pli_shards[i]);
        for (std::size_t j = i + 1; j != pli_shards.size(); ++j) {
            CalculateCrossMinMaxDif(pli_shards[i], pli_shards[j]);
        }
    }
}

void MinMaxDifCalculator::CalculateSingleMinMaxDif(PliShard const& pli_shard) {
    std::size_t const num_columns = pli_shard.Plis().size();
    for (model::ColumnIndex column_index = 0; column_index != num_columns; ++column_index) {
        Pli const& pli = pli_shard.Plis()[column_index];
        bool const is_distance_ordered = distance_calculator_->IsDistanceOrdered(column_index);
        for (std::size_t i = 0; i != pli.Size(); ++i) {
            if (pli.GetCluster(i).size() > 1) {
                min_max_dif_[column_index].lower_bound =
                        std::min(min_max_dif_[column_index].lower_bound, 0.0);
                if (distance_calculator_->IsDistanceOrdered(column_index)) {
                    break;
                }
            }
            if (!is_distance_ordered) {
                for (std::size_t j = i + 1; j != pli.Size(); ++j) {
                    double const diff = distance_calculator_->CalculateDistance(
                            column_index, {pli.GetCluster(i)[0], pli.GetCluster(j)[0]});
                    min_max_dif_[column_index].lower_bound =
                            std::min(min_max_dif_[column_index].lower_bound, diff);
                    min_max_dif_[column_index].upper_bound =
                            std::max(min_max_dif_[column_index].upper_bound, diff);
                }
            } else {
                if (i != pli.Size() - 1) {
                    double const diff = distance_calculator_->CalculateDistance(
                            column_index, {pli.GetCluster(i)[0], pli.GetCluster(i + 1)[0]});
                    min_max_dif_[column_index].lower_bound =
                            std::min(min_max_dif_[column_index].lower_bound, diff);
                }
            }
        }
        if (is_distance_ordered) {
            double const diff = distance_calculator_->CalculateDistance(
                    column_index, {pli.GetCluster(0)[0], pli.GetCluster(pli.Size() - 1)[0]});
            min_max_dif_[column_index].upper_bound =
                    std::max(min_max_dif_[column_index].upper_bound, diff);
        }
    }
}

void MinMaxDifCalculator::CalculateCrossMinMaxDif(PliShard const& first_pli_shard,
                                                  PliShard const& second_pli_shard) {
    std::size_t const num_columns = first_pli_shard.Plis().size();
    for (model::ColumnIndex column_index = 0; column_index != num_columns; ++column_index) {
        Pli const& first_pli = first_pli_shard.Plis()[column_index];
        Pli const& second_pli = second_pli_shard.Plis()[column_index];
        bool const is_distance_ordered = distance_calculator_->IsDistanceOrdered(column_index);
        std::size_t cur_left_bound = 0;
        for (std::size_t i = 0; i != first_pli.Size(); ++i) {
            if (!is_distance_ordered) {
                for (std::size_t j = 0; j != second_pli.Size(); ++j) {
                    double const diff = distance_calculator_->CalculateDistance(
                            column_index,
                            {first_pli.GetCluster(i)[0], second_pli.GetCluster(j)[0]});
                    min_max_dif_[column_index].lower_bound =
                            std::min(min_max_dif_[column_index].lower_bound, diff);
                    min_max_dif_[column_index].upper_bound =
                            std::max(min_max_dif_[column_index].upper_bound, diff);
                }
            } else {
                std::size_t left_bound = cur_left_bound;
                std::size_t right_bound = second_pli.Size() + 1;
                while (right_bound - left_bound > 1) {
                    std::size_t const mid = (left_bound + right_bound) / 2;
                    model::CompareResult comp_res = distance_calculator_->Compare(
                            column_index,
                            {second_pli.GetCluster(mid - 1)[0], first_pli.GetCluster(i)[0]});
                    if (comp_res == model::CompareResult::kLess) {
                        right_bound = mid;
                    } else {
                        left_bound = mid;
                    }
                }
                if (right_bound - 1 < second_pli.Size()) {
                    double const diff = distance_calculator_->CalculateDistance(
                            column_index, {first_pli.GetCluster(i)[0],
                                           second_pli.GetCluster(right_bound - 1)[0]});
                    min_max_dif_[column_index].lower_bound =
                            std::min(min_max_dif_[column_index].lower_bound, diff);
                }
                if (left_bound >= 1) {
                    double const diff = distance_calculator_->CalculateDistance(
                            column_index,
                            {first_pli.GetCluster(i)[0], second_pli.GetCluster(left_bound - 1)[0]});
                    min_max_dif_[column_index].lower_bound =
                            std::min(min_max_dif_[column_index].lower_bound, diff);
                }
                cur_left_bound = right_bound - 1;
            }
            if (is_distance_ordered) {
                double const first_diff = distance_calculator_->CalculateDistance(
                        column_index, {first_pli.GetCluster(i)[0], second_pli.GetCluster(0)[0]});
                min_max_dif_[column_index].upper_bound =
                        std::max(min_max_dif_[column_index].upper_bound, first_diff);
                double const second_diff = distance_calculator_->CalculateDistance(
                        column_index, {first_pli.GetCluster(i)[0],
                                       second_pli.GetCluster(second_pli.Size() - 1)[0]});
                min_max_dif_[column_index].upper_bound =
                        std::max(min_max_dif_[column_index].upper_bound, second_diff);
            }
        }
    }
}

}  // namespace algos::dd
