#pragma once

#include <memory>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dc/dc_verifier/dc_verifier.h"
#include "core/algorithms/dc/measures/measure.h"

namespace algos {

class ADCVerifier final : public Algorithm {
private:
    std::shared_ptr<algos::DCVerifier> verifier_;
    dc::MeasureType measure_type_ = dc::MeasureType::G1;
    std::string dc_string_;
    config::InputTable input_table_;
    double error_treshold_;
    std::optional<double> error_;

    void LoadDataInternal() override;

    void MakeExecuteOptsAvailable() override;

    void RegisterOptions();

    void ResetState() override {};

    unsigned long long ExecuteInternal() override;

public:
    ADCVerifier();

    bool ADCHolds() const {
        if (!error_.has_value())
            throw std::logic_error("ADCVerifier::ADCHolds() must be called after Execute()");
        return *error_ <= error_treshold_;
    }

    double GetError() const noexcept {
        return *error_;
    }

    auto GetViolations() const noexcept {
        return verifier_->GetViolations();
    }
};

}  // namespace algos
