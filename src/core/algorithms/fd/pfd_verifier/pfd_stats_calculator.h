#pragma once

#include "core/config/error/type.h"
#include "core/config/error_measure/type.h"
#include "core/config/indices/type.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/position_list_index.h"

namespace algos {

class PFDStatsCalculator {
private:
    std::shared_ptr<ColumnLayoutRelationData> relation_;
    config::PfdErrorMeasureType error_measure_;

    std::vector<model::PLI::Cluster> clusters_violating_pfd_;
    size_t num_rows_violating_pfd_ = 0;
    config::ErrorType error_ = 0.0;

public:
    explicit PFDStatsCalculator(std::shared_ptr<ColumnLayoutRelationData> relation,
                                config::PfdErrorMeasureType measure)
        : relation_(std::move(relation)), error_measure_(measure) {}

    void ResetState() {
        clusters_violating_pfd_.clear();
        num_rows_violating_pfd_ = 0;
        error_ = 0;
    }

    size_t GetNumViolatingClusters() const {
        return clusters_violating_pfd_.size();
    }

    size_t GetNumViolatingRows() const {
        return num_rows_violating_pfd_;
    }

    config::ErrorType GetError() const {
        return error_;
    }

    std::vector<model::PLI::Cluster> const& GetViolatingClusters() const {
        return clusters_violating_pfd_;
    }

    void CalculateStatistics(model::PositionListIndex const* x_pli,
                             model::PositionListIndex const* xa_pli) {
        using Cluster = model::PLI::Cluster;
        std::deque<Cluster> xa_index = xa_pli->GetIndex();
        std::shared_ptr<Cluster const> probing_table = x_pli->CalculateAndGetProbingTable();
        std::sort(xa_index.begin(), xa_index.end(),
                  [&probing_table](Cluster const& a, Cluster const& b) {
                      return probing_table->at(a.front()) < probing_table->at(b.front());
                  });
        double sum = 0.0;
        std::size_t cluster_rows_count = 0;
        std::deque<Cluster> const& x_index = x_pli->GetIndex();
        auto xa_cluster_it = xa_index.begin();

        for (Cluster const& x_cluster : x_index) {
            std::size_t max = 1;
            std::size_t x_cluster_size = x_cluster.size();
            for (int x_row : x_cluster) {
                if (xa_cluster_it == xa_index.end()) {
                    break;
                }
                if (x_row == xa_cluster_it->at(0)) {
                    max = std::max(max, xa_cluster_it->size());
                    xa_cluster_it++;
                }
            }
            if (max != x_cluster_size) {
                clusters_violating_pfd_.push_back(x_cluster);
            }
            num_rows_violating_pfd_ += x_cluster_size - max;
            sum += error_measure_ == +PfdErrorMeasure::per_tuple
                           ? static_cast<double>(max)
                           : static_cast<double>(max) / x_cluster_size;
            cluster_rows_count += x_cluster.size();
        }
        unsigned int unique_rows =
                static_cast<unsigned int>(x_pli->GetRelationSize() - cluster_rows_count);
        double probability =
                static_cast<double>(sum + unique_rows) /
                (error_measure_ == +PfdErrorMeasure::per_tuple ? x_pli->GetRelationSize()
                                                               : x_index.size() + unique_rows);
        error_ = 1.0 - probability;
    }
};

}  // namespace algos
