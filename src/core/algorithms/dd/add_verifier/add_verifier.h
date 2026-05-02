#pragma once

#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"

namespace algos::dd {
using DFs = model::DFStringConstraint;
using DDs = model::DDString;

class ADDVerifier : public DDVerifier {
private:
    double satisfaction_threshold_;
    void CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const& lhs) override;
    void RegisterOptions();
    void CheckCorrectnessDd() const override;

protected:
    void MakeExecuteOptsAvailable() override;

public:
    ADDVerifier();
    bool DDHolds() const override;
};
}  // namespace algos::dd
