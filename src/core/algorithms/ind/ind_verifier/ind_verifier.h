#pragma once

#include "algorithms/algorithm.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "error/type.h"
#include "table/tuple_index.h"
#include "tabular_data/input_tables_type.h"

namespace algos {

/// Algorithm for verifying IND
class INDVerifier : public Algorithm {
public:
    struct RawIND {
        config::IndicesType lhs;
        config::IndicesType rhs;
    };

    using Cluster = std::vector<model::TupleIndex>;
    using Error = config::ErrorType;

private:
    config::InputTables input_tables_;

    /* IND to verify */
    RawIND ind_;
    config::EqNullsType is_null_equal_null_;
    Error error_;

    std::vector<Cluster> clusters_{};
    model::TupleIndex violating_rows_{};
    model::TupleIndex violating_unique_rows_{};

    void RegisterOptions();

    void ResetState() final;

    void VerifyIND();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    explicit INDVerifier();

    bool Holds() const noexcept {
        return error_ == 0.0;
    }

    Error GetError() const noexcept {
        return error_;
    }

#if 0
    ///
    /// Get the number of clusters where the IND is violated, that is, the number of sets of rows
    /// where each set consists of rows equal to each other in the LHS.
    ///
    size_t GetViolatingClustersCount() const noexcept {
        return clusters_.size();
    }
#endif

    /// Get the total number of table rows that violate the IND.
    size_t GetViolatingRowsCount() const noexcept {
        return violating_rows_;
    }

    /// Returns the total number of table rows (with unique lhs part) that violate the IND.
    size_t GetViolatingUniqueRowsCount() const noexcept {
        return GetViolatingClusters().size();
    }

    ///
    /// Get clusters where the IND is violated, that is, sets of rows where each set consists of
    /// rows equal to each other in the specified columns.
    ///
    std::vector<Cluster> const& GetViolatingClusters() const noexcept {
        return clusters_;
    }
};

}  // namespace algos
