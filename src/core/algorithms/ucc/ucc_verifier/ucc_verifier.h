#pragma once

#include <vector>

#include "algorithms/algorithm.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"

namespace algos {

/* Algorithm used to verify that a set of columns is UCC and retrieving useful information in
 * case it is not */
class UCCVerifier : public Algorithm {
private:
    /* input options */
    config::IndicesType column_indices_;
    config::EqNullsType is_null_equal_null_{};

    config::InputTable input_table_;
    std::unique_ptr<ColumnLayoutRelationData> relation_;

    /* results of work */
    size_t num_rows_violating_ucc_ = 0;
    std::vector<model::PLI::Cluster> clusters_violating_ucc_;

    void VerifyUCC();
    void CalculateStatistics(std::deque<model::PLI::Cluster> const& clusters);
    void RegisterOptions();
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

    void ResetState() override {
        clusters_violating_ucc_.clear();
        num_rows_violating_ucc_ = 0;
    }

public:
    /* Returns true if UCC holds and false otherwise */
    bool UCCHolds() const {
        return clusters_violating_ucc_.empty();
    }

    /* Returns the number of clusters where the UCC is violated, that is, the number of sets of rows
     * where each set consists of rows equal to each other in the specified columns */
    size_t GetNumClustersViolatingUCC() const {
        return clusters_violating_ucc_.size();
    }

    /* Returns the total number of table rows that violate the UCC */
    size_t GetNumRowsViolatingUCC() const {
        return num_rows_violating_ucc_;
    }

    /* Returns clusters where the UCC is violated, that is, sets of rows where each set consists of
     * rows equal to each other in the specified columns */
    std::vector<model::PLI::Cluster> const& GetClustersViolatingUCC() const {
        return clusters_violating_ucc_;
    }

    UCCVerifier();
};

}  // namespace algos
