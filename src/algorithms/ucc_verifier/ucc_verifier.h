#pragma once

#include <cassert>
#include <deque>
#include <string>
#include <vector>

#include "algorithms/ucc_verifier/stats_calculator.h"
#include "algorithms/options/equal_nulls/type.h"
#include "algorithms/options/indices/type.h"
#include "algorithms/primitive.h"

namespace algos::ucc_verifier {
//add actual description <<--
/* Primitive used for verifying a particular FD and retrieving useful information about this FD in
 * case it doesn't hold */
class UCCVerifier : public Primitive {
private:
    config::IndicesType column_indices_;
    config::EqNullsType is_null_equal_null_;

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
    void FitInternal(model::IDatasetStream& data_stream) override;
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

    //
    std::vector<util::PLI::Cluster> const& GetErrorClusters() const {
        return stats_calculator_->GetErrorClusters();
    }

    void SortHighlightsByProportionAscending() const;
    void SortHighlightsByProportionDescending() const;
    void SortHighlightsByNumAscending() const;
    void SortHighlightsByNumDescending() const;
    void SortHighlightsBySizeAscending() const;
    void SortHighlightsBySizeDescending() const;
    void SortHighlightsByLhsAscending() const;
    void SortHighlightsByLhsDescending() const;
    /*
    StatsCalculator::ClusterCompareFunction CompareHighlightsByLhsDescending() const {
    //    assert(stats_calculator_);
    //    return stats_calculator_->CompareHighlightsByLhsDescending();
    }

    StatsCalculator::ClusterCompareFunction CompareHighlightsByLhsAscending() const {
    //    assert(stats_calculator_);
    //    return stats_calculator_->CompareHighlightsByLhsAscending();
    }
    */
    UCCVerifier();
};

}  // namespace algos::ucc_verifier
