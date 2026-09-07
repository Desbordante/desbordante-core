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
    double CalculateError(boost::dynamic_bitset<> const& key_candidate) const override;
    DependencyCandidate CreateDependencyCandidate(
            boost::dynamic_bitset<> const& vertical) const override;

    void RegisterDependency(boost::dynamic_bitset<> const& vertical, double error,
                            DependencyConsumer const& discovery_unit) const override;

    bool IsIrrelevantColumn([[maybe_unused]] unsigned int column_index) const override {
        return false;
    }

    unsigned int GetNumIrrelevantColumns() const override {
        return 1;
    }

    boost::dynamic_bitset<> GetIrrelevantColumns() const override {
        return boost::dynamic_bitset<>(
                context_->GetColumnLayoutRelationData()->GetSchema()->GetNumColumns());
    }

    std::unique_ptr<DependencyStrategy> CreateClone() override;
};
