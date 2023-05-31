#pragma once

#include <cassert>
#include <deque>
#include <string>
#include <vector>

#include "algorithms/ucc_verifier/stats_calculator.h"
#include "util/config/equal_nulls/type.h"
#include "util/config/indices/type.h"
#include "algorithms/algorithm.h"

namespace algos::ucc_verifier {

/* Primitive used for verifying a particular UCC and retrieving useful information about this UCC in
 * case it's not valid */
class UCCVerifier : public Algorithm {
private:
    util::config::IndicesType column_indices_;
    util::config::EqNullsType is_null_equal_null_;

    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::unique_ptr<StatsCalculator> stats_calculator_;

    void VerifyUCC() const;
    void RegisterOptions();
    void ResetState() final {
        if (stats_calculator_) {
            stats_calculator_->ResetState();
        }
    }

protected:
    void LoadDataInternal(model::IDatasetStream& data_stream) override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    /* Returns true if FD holds */
    bool UCCHolds() const {
        assert(stats_calculator_);
        return stats_calculator_->UCCHolds();
    }

    /* Returns the number of clusters where UCC is violated */
    size_t GetNumErrorClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumErrorClusters();
    }

    /* Returns the number of rows that violate the UCC */
    size_t GetNumErrorRows() const {
        return stats_calculator_->GetNumErrorRows();
    }

    std::vector<util::PLI::Cluster> const& GetErrorClusters() const {
        return stats_calculator_->GetErrorClusters();
    }

    UCCVerifier();
};

}  // namespace algos::ucc_verifier
