#pragma once

#include <boost/format.hpp>

#include "core/algorithms/fd/pyrocommon/core/dependency_strategy.h"

class FdG1Strategy : public DependencyStrategy {
private:
    model::Index rhs_index_;

    double CalculateG1(model::PositionListIndex const* lhs_pli) const;
    double CalculateG1(double num_violating_tuple_pairs) const;
    model::ConfidenceInterval CalculateG1(model::ConfidenceInterval const& num_violations) const;

public:
    FdG1Strategy(model::Index rhs_index, double max_error, double deviation)
        : DependencyStrategy(max_error, deviation), rhs_index_(rhs_index) {}

    void EnsureInitialized(SearchSpace* search_space) const override;
    double CalculateError(boost::dynamic_bitset<> const& lhs) const override;
    DependencyCandidate CreateDependencyCandidate(
            boost::dynamic_bitset<> const& vertical) const override;

    // TODO: can it be const though? Dependency registers --> some state somewhere changes.
    // Non-const discovery_unit?
    void RegisterDependency(boost::dynamic_bitset<> const& vertical, double error,
                            DependencyConsumer const& discovery_unit) const override;

    bool IsIrrelevantColumn(unsigned int column_index) const override {
        return rhs_index_ == column_index;
    }

    unsigned int GetNumIrrelevantColumns() const override {
        return 1;
    }

    std::unique_ptr<DependencyStrategy> CreateClone() override;

    boost::dynamic_bitset<> GetIrrelevantColumns() const override {
        return boost::dynamic_bitset<>(context_->GetSchema()->GetNumColumns()).set(rhs_index_);
    }
};
