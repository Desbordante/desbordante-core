#include "algorithms/primitive.h"

namespace algos {

bool Primitive::FitCompleted() const {
    return fit_completed_;
}

Primitive::Primitive(std::vector<std::string_view> phase_names)
    : progress_(std::move(phase_names)) {}

void Primitive::Fit(model::IDatasetStream& data) {
    if (configuration_.GetCurrentStage() != +config::ConfigurationStage::fit)
        throw std::logic_error("Incorrect algorithm execution order: Fit.");
    if (!GetNeededOptions().empty())
        throw std::logic_error("All options need to be set before starting processing.");
    FitInternal(data);
    data.Reset();
    configuration_.StartStage(config::ConfigurationStage::execute);
}

unsigned long long Primitive::Execute() {
    if (configuration_.GetCurrentStage() != +config::ConfigurationStage::execute)
        throw std::logic_error("Incorrect algorithm execution order: Execute.");
    if (!GetNeededOptions().empty())
        throw std::logic_error("All options need to be set before execution.");
    progress_.ResetProgress();
    ResetState();
    auto time_ms = ExecuteInternal();
    configuration_.StartStage(config::ConfigurationStage::execute);
    return time_ms;
}

}  // namespace algos
