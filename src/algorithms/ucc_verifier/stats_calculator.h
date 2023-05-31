#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "util/config/indices/type.h"
#include "model/column_layout_relation_data.h"
#include "model/column_layout_typed_relation_data.h"

namespace algos::ucc_verifier {

class StatsCalculator {
private:
    using ClusterIndex = util::PLI::Cluster::value_type;

    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    util::config::IndicesType column_indices_;

    size_t num_error_rows_ = 0;
    std::vector<util::PLI::Cluster> err_clusters_;

    void VisualizeClusters() const;
    std::string GetStringValue(ClusterIndex row_index) const;
    std::string GetStringValueByIndex(ClusterIndex row_index, ClusterIndex col_index) const;

public:
    using ClusterCompareFunction =
            std::function<bool(util::PLI::Cluster const& h1, util::PLI::Cluster const& h2)>;

    void CalculateStatistics(std::deque<util::PLI::Cluster> clusters);

    void PrintStatistics() const;

    void ResetState() {
        err_clusters_.clear();
        num_error_rows_ = 0;
    }

    bool UCCHolds() const {
        return err_clusters_.empty();
    }

    size_t GetNumErrorClusters() const {
        return err_clusters_.size();
    }

    size_t GetNumErrorRows() const {
        return num_error_rows_;
    }

    std::vector<util::PLI::Cluster> const& GetErrorClusters() const {
        return err_clusters_;
    }
    explicit StatsCalculator(std::shared_ptr<ColumnLayoutRelationData> relation,
                             std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
                             util::config::IndicesType column_indices)
        : relation_(std::move(relation)),
          typed_relation_(std::move(typed_relation)),
          column_indices_(std::move(column_indices))
        {}
};

}  // namespace algos::ucc_verifier