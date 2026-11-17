#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/cdd/cdd.h"
#include "core/algorithms/cdd/cdd_verifier/cdd_highlight.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"

namespace algos::cdd {

class CDDVerifier : public dd::DDVerifier {
private:
    model::CDD cdd_;

    void RegisterOptions();

    void ResetState() override {
        error_ = 0.;
        num_error_rhs_ = 0;
        highlights_.clear();
        lhs_column_indices_.clear();
        rhs_column_indices_.clear();
        ids_.clear();
        cdd_highlights_.clear();
        row_satisfies_rhs_.clear();
        added_to_cdd_highlights_.clear();
    }

    std::vector<std::size_t> GetIndicesPruningTable();

protected:
    void MakeExecuteOptsAvailable() override;

    void ExecuteInternal() override;

    void CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const& lhs) override;

public:
    CDDVerifier();

    std::vector<CDDHighlight> const& GetCddHighlights() const {
        return cdd_highlights_;
    }

    std::size_t GetNumCondViolations() const {
        return cdd_highlights_.size();
    }

    bool DDHolds() const override;

private:
    std::vector<CDDHighlight> cdd_highlights_;
    std::vector<bool> row_satisfies_rhs_;
    std::vector<bool> added_to_cdd_highlights_;
};
}  // namespace algos::cdd
