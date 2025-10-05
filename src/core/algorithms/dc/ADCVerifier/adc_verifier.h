#pragma once

#include <memory>

#include "algorithms/algorithm.h"
#include "algorithms/dc/DCVerifier/dc_verifier.h"
#include "algorithms/dc/measures/measure.h"

namespace algos {

class ADCVerifier final : public Algorithm {
private:
    std::shared_ptr<algos::DCVerifier> verifier_;
    dc::MeasureType measure_type_ = dc::MeasureType::G1;
    // std::string measure_type_str_;
    std::string dc_string_;
    config::InputTable input_table_;
    double error_;
    bool holds_;

    // void SetMeasure(std::string str);

    void LoadDataInternal() override;

    void MakeExecuteOptsAvailable() override;

    void RegisterOptions();

    void ResetState() override {};

    unsigned long long ExecuteInternal() override;

public:
    ADCVerifier();

    bool GetResult() {
        return holds_;
    }
};

}  // namespace algos
