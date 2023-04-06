#include "algorithms/primitive.h"

namespace algos {

Primitive::Primitive(std::vector<std::string_view> phase_names)
    : progress_(std::move(phase_names)) {}

void Primitive::Fit(model::IDatasetStream& data) {
    FitCommon<model::IDatasetStream&>(data, [this](model::IDatasetStream& data) {
        FitInternal(data);
    });
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
