#pragma once

#include <cassert>
#include <deque>
#include <string>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/fd/fd_verifier/stats_calculator.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"

namespace algos::fd_verifier {

/* Algorithm used for verifying a particular FD and retrieving useful information about this FD in
 * case it doesn't hold */
class FDVerifier : public Algorithm {
private:
    util::config::InputTable input_table_;

    util::config::IndicesType lhs_indices_;
    util::config::IndicesType rhs_indices_;
    util::config::EqNullsType is_null_equal_null_;

    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::unique_ptr<StatsCalculator> stats_calculator_;

    void VerifyFD() const;
    std::shared_ptr<util::PLI const> CalculatePLI(util::config::IndicesType const& indices) const;
    void RegisterOptions();
    void ResetState() final {
        if (stats_calculator_) {
            stats_calculator_->ResetState();
        }
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    /* Returns true if FD holds */
    bool FDHolds() const {
        assert(stats_calculator_);
        return stats_calculator_->FDHolds();
    }

    /* Returns the number of clusters where FD is violated */
    size_t GetNumErrorClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumErrorClusters();
    }

    /* Returns the number of rows that violate the FD */
    size_t GetNumErrorRows() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumErrorRows();
    }

    /* Returns the error threshold at which AFD holds */
    long double GetError() const {
        assert(stats_calculator_);
        return stats_calculator_->GetError();
    }

    /* Returns highlights */
    std::vector<Highlight> const& GetHighlights() const {
        assert(stats_calculator_);
        return stats_calculator_->GetHighlights();
    }

    void SortHighlightsByProportionAscending() const;
    void SortHighlightsByProportionDescending() const;
    void SortHighlightsByNumAscending() const;
    void SortHighlightsByNumDescending() const;
    void SortHighlightsBySizeAscending() const;
    void SortHighlightsBySizeDescending() const;
    void SortHighlightsByLhsAscending() const;
    void SortHighlightsByLhsDescending() const;

    StatsCalculator::HighlightCompareFunction CompareHighlightsByLhsDescending() const {
        assert(stats_calculator_);
        return stats_calculator_->CompareHighlightsByLhsDescending();
    }

    StatsCalculator::HighlightCompareFunction CompareHighlightsByLhsAscending() const {
        assert(stats_calculator_);
        return stats_calculator_->CompareHighlightsByLhsAscending();
    }

    FDVerifier();
};

}  // namespace algos::fd_verifier
