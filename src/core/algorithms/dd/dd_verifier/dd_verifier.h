#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithm.h"
#include "algorithms/dd/dd.h"
#include "algorithms/dd/dd_verifier/highlight.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::dd {
using DFs = model::DFStringConstraint;
using DDs = model::DDString;

class DDVerifier : public Algorithm {
private:
    DDs dd_;
    config::InputTable input_table_;
    std::size_t num_rows_{};
    std::size_t num_columns_{};
    std::size_t num_error_rhs_{};
    std::vector<model::ColumnIndex> lhs_column_indices_;
    std::vector<model::ColumnIndex> rhs_column_indices_;
    double error_ = 0.;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::vector<Highlight> highlights_;

    void RegisterOptions();

    void VisualizeHighlights() const;

    void PrintStatistics() const;

    std::vector<std::pair<std::size_t, std::size_t>> GetRowsWhereLhsHolds() const;

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> const &tuple_pair) const;

    void CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const &lhs);

    void VerifyDD();

    bool IsColumnMetrizable(model::ColumnIndex const column_index) const;

    void CheckCorrectnessDd() const;

    void ResetState() final {
        error_ = 0.;
        num_error_rhs_ = 0;
        highlights_.clear();
        lhs_column_indices_.clear();
        rhs_column_indices_.clear();
    }

protected:
    void LoadDataInternal() override;

    void MakeExecuteOptsAvailable() override;

    unsigned long long ExecuteInternal() override;

public:
    DDVerifier();

    double GetError() const;

    std::size_t GetNumErrorRhs() const;

    bool DDHolds() const;

    std::vector<Highlight> const &GetHighlights() const;
};
}  // namespace algos::dd
