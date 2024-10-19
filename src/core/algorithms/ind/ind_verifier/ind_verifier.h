#pragma once

#include "algorithms/algorithm.h"
#include "config/indices/type.h"
#include "error/type.h"
#include "table/tuple_index.h"
#include "tabular_data/input_tables_type.h"

namespace algos {

/// Algorithm for verifying AIND
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
    Error error_;

    std::vector<Cluster> clusters_{};
    model::TupleIndex violating_rows_{};

    void RegisterOptions();

    void ResetState() final;

    void VerifyIND();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    explicit INDVerifier();

    /// Check if exact IND holds.
    bool Holds() const noexcept {
        return error_ == 0.0;
    }

    /// Get the error threshold at which AIND holds. */
    Error GetError() const noexcept {
        return error_;
    }

    /// Get the total number of table rows that violate the IND.
    size_t GetViolatingRowsCount() const noexcept {
        return violating_rows_;
    }

    ///
    /// Get the number of clusters where the IND is violated, that is, the number
    /// of unique rows in the specified columns that need to be removed.
    ///
    size_t GetViolatingClustersCount() const noexcept {
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
