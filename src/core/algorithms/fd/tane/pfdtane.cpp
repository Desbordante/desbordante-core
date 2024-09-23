#include "pfdtane.h"

#include "config/error/option.h"
#include "config/error_measure/option.h"
#include "enums.h"
#include "fd/pli_based_fd_algorithm.h"
#include "model/table/column_data.h"

namespace algos {
using Cluster = model::PositionListIndex::Cluster;

void PFDTane::RegisterOptions() {
    RegisterOption(config::kErrorMeasureOpt(&error_measure_));
}

void PFDTane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::kErrorMeasureOpt.GetName()});
}

PFDTane::PFDTane(std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : tane::TaneCommon(relation_manager) {
    RegisterOptions();
}

config::ErrorType PFDTane::CalculateZeroAryFdError(ColumnData const* rhs) {
    return CalculateZeroAryPFDError(rhs);
}

config::ErrorType PFDTane::CalculateFdError(model::PositionListIndex const* lhs_pli,
                                            model::PositionListIndex const* joint_pli) {
    return CalculatePFDError(lhs_pli, joint_pli, error_measure_);
}

config::ErrorType PFDTane::CalculateZeroAryPFDError(ColumnData const* rhs) {
    std::size_t max = 1;
    model::PositionListIndex const* x_pli = rhs->GetPositionListIndex();
    for (Cluster const& x_cluster : x_pli->GetIndex()) {
        max = std::max(max, x_cluster.size());
    }
    return 1.0 - static_cast<double>(max) / x_pli->GetRelationSize();
}

config::ErrorType PFDTane::CalculatePFDError(model::PositionListIndex const* x_pli,
                                             model::PositionListIndex const* xa_pli,
                                             ErrorMeasure measure) {
    std::deque<Cluster> xa_index = xa_pli->GetIndex();
    std::shared_ptr<Cluster const> probing_table_ptr = x_pli->CalculateAndGetProbingTable();
    auto const& probing_table = *probing_table_ptr;
    std::sort(xa_index.begin(), xa_index.end(),
              [&probing_table](Cluster const& a, Cluster const& b) {
                  return probing_table[a.front()] < probing_table[b.front()];
              });
    double sum = 0.0;
    std::size_t cluster_rows_count = 0;
    std::deque<Cluster> const& x_index = x_pli->GetIndex();
    auto xa_cluster_it = xa_index.begin();
    for (Cluster const& x_cluster : x_index) {
        std::size_t max = 1;
        for (int x_row : x_cluster) {
            if (xa_cluster_it == xa_index.end()) {
                break;
            }
            auto const& xa_cluster = *xa_cluster_it;
            if (x_row == xa_cluster[0]) {
                max = std::max(max, xa_cluster_it->size());
                xa_cluster_it++;
            }
        }
        sum += measure == +ErrorMeasure::per_tuple ? static_cast<double>(max)
                                                   : static_cast<double>(max) / x_cluster.size();
        cluster_rows_count += x_cluster.size();
    }
    unsigned int unique_rows =
            static_cast<unsigned int>(x_pli->GetRelationSize() - cluster_rows_count);
    double probability = static_cast<double>(sum + unique_rows) /
                         (measure == +ErrorMeasure::per_tuple ? x_pli->GetRelationSize()
                                                              : x_index.size() + unique_rows);
    return 1.0 - probability;
}

}  // namespace algos
