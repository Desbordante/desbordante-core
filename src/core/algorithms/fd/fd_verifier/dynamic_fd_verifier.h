#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/fd/fd_verifier/dynamic_stats_calculator.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/dynamic_table_data.h"
#include "table/dynamic_position_list_index.h"
#include "table/idataset_stream.h"

namespace algos {
namespace fd_verifier {
class Highlight;
}  // namespace fd_verifier
}  // namespace algos

namespace model {
struct DynamicTableData;
}  // namespace model

namespace algos::fd_verifier {

/* Algorithm used for verifying a particular FD and retrieving useful information about this FD in
 * case it doesn't hold. Algorithm works only for one FD, that we define during initialization,
 * and monitor it in the process of changing the table */
class DynamicFDVerifier : public Algorithm {
private:
    config::InputTable input_table_;
    config::InputTable insert_statements_table_ = nullptr;
    config::InputTable update_statements_table_ = nullptr;
    std::unordered_set<size_t> delete_statement_indices_;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    std::shared_ptr<model::DynPLI> lhs_pli_;
    std::shared_ptr<model::DynPLI> rhs_pli_;

    std::shared_ptr<model::DynamicTableData> table_data_;
    std::shared_ptr<DynamicStatsCalculator> stats_calculator_;

    std::unordered_map<std::string, int> value_dictionary_{};
    int next_value_id_ = 1;
    static constexpr int kNullValueId = -1;

    void VerifyFD() const;
    void CreateFD();

    inline std::vector<int> ParseRowForPLI(model::IDatasetStream::Row::iterator const& row_begin,
                                           std::vector<unsigned int> const& indices);
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

    DynamicFDVerifier();
};

}  // namespace algos::fd_verifier
