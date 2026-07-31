#pragma once
#include <boost/format.hpp>

#include "core/algorithms/fd/pyrocommon/core/dependency_strategy.h"

class KeyG1Strategy : public DependencyStrategy {
private:
    double CalculateKeyError(model::PositionListIndex const* pli) const;
    double CalculateKeyError(double num_violating_tuple_pairs) const;
    model::ConfidenceInterval CalculateKeyError(
            model::ConfidenceInterval const& num_violations) const;

public:
    KeyG1Strategy(double max_error, double deviation) : DependencyStrategy(max_error, deviation) {}

    void EnsureInitialized(SearchSpace* search_space) const override;
    double CalculateError(Vertical const& key_candidate) const override;
    DependencyCandidate CreateDependencyCandidate(Vertical const& vertical) const override;

    void RegisterDependency(Vertical const& vertical, double error,
                            DependencyConsumer const& discovery_unit) const override;

    bool IsIrrelevantColumn([[maybe_unused]] unsigned int column_index) const override {
        return false;
    }

    unsigned int GetNumIrrelevantColumns() const override {
        return 1;
    }

    Vertical GetIrrelevantColumns() const override {
        return context_->GetColumnLayoutRelationData()->GetSchema()->CreateEmptyVertical();
    }

    std::unique_ptr<DependencyStrategy> CreateClone() override;
};
