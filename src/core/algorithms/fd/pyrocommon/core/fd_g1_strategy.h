#pragma once

#include <boost/format.hpp>

#include "core/algorithms/fd/pyrocommon/core/dependency_strategy.h"

class FdG1Strategy : public DependencyStrategy {
private:
    Column const* rhs_;

    double CalculateG1(model::PositionListIndex const* lhs_pli) const;
    double CalculateG1(double num_violating_tuple_pairs) const;
    model::ConfidenceInterval CalculateG1(model::ConfidenceInterval const& num_violations) const;

public:
    FdG1Strategy(Column const* rhs, double max_error, double deviation)
        : DependencyStrategy(max_error, deviation), rhs_(rhs) {}

    void EnsureInitialized(SearchSpace* search_space) const override;
    double CalculateError(Vertical const& lhs) const override;
    DependencyCandidate CreateDependencyCandidate(Vertical const& vertical) const override;

    // TODO: can it be const though? Dependency registers --> some state somewhere changes.
    // Non-const discovery_unit?
    void RegisterDependency(Vertical const& vertical, double error,
                            DependencyConsumer const& discovery_unit) const override;

    bool IsIrrelevantColumn(unsigned int column_index) const override {
        return rhs_->GetIndex() == column_index;
    }

    unsigned int GetNumIrrelevantColumns() const override {
        return 1;
    }

    std::unique_ptr<DependencyStrategy> CreateClone() override;

    Vertical GetIrrelevantColumns() const override {
        return static_cast<Vertical>(*rhs_);
    }
};
