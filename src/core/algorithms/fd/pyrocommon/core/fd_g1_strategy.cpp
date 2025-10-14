#include "fd_g1_strategy.h"

#include <unordered_map>

#include <easylogging++.h>

#include "../model/pli_cache.h"
#include "search_space.h"

unsigned long long FdG1Strategy::nanos_ = 0;

double FdG1Strategy::CalculateG1(model::PositionListIndex* lhs_pli) const {
    unsigned long long num_violations = 0;
    std::unordered_map<int, int> value_counts;
    std::vector<int> const& probing_table = context_->GetColumnLayoutRelationData()
                                                    ->GetColumnData(rhs_->GetIndex())
                                                    .GetProbingTable();

    LOG(DEBUG) << boost::format{"Probing table size for %1%: %2%"} % rhs_->ToString() %
                          std::to_string(probing_table.size());

    // Perform probing
    int probing_table_value_id;
    for (auto const& cluster : lhs_pli->GetIndex()) {
        value_counts.clear();
        for (int position : cluster) {
            probing_table_value_id = probing_table[position];
            if (probing_table_value_id != model::PositionListIndex::kSingletonValueId) {
                value_counts[probing_table_value_id] += 1;
            }
        }

        unsigned long long num_violations_in_cluster = cluster.size() * (cluster.size() - 1) / 2;
        for (auto const& [key, refined_cluster_size] : value_counts) {
            num_violations_in_cluster -= static_cast<unsigned long long>(refined_cluster_size) *
                                         (refined_cluster_size - 1) / 2;
        }
        num_violations += num_violations_in_cluster;
    }

    return CalculateG1(num_violations);
}

double FdG1Strategy::CalculateG1(double num_violating_tuple_pairs) const {
    unsigned long long num_tuple_pairs =
            context_->GetColumnLayoutRelationData()->GetNumTuplePairs();
    if (num_tuple_pairs == 0) return 0;
    double g1 = num_violating_tuple_pairs / num_tuple_pairs;
    return Round(g1);
}

void FdG1Strategy::EnsureInitialized(SearchSpace* search_space) const {
    if (search_space->is_initialized_) return;

    Vertical empty_vertical =
            context_->GetColumnLayoutRelationData()->GetSchema()->CreateEmptyVertical();
    double zero_fd_error = CalculateError(empty_vertical);
    search_space->AddLaunchPad(DependencyCandidate(std::move(empty_vertical),
                                                   model::ConfidenceInterval(zero_fd_error), true));

    search_space->is_initialized_ = true;
}

double FdG1Strategy::CalculateError(Vertical const& lhs) const {
    double error = 0;
    if (lhs.GetArity() == 0) {
        auto rhs_pli = context_->GetPliCache()->Get(static_cast<Vertical>(*rhs_));
        if (rhs_pli == nullptr) {
            throw std::runtime_error(
                    "Couldn't get rhsPLI from PLICache while calculating FD error");
        }
        error = CalculateG1(rhs_pli->GetNip());
    } else {
        auto lhs_pli = context_->GetPliCache()->GetOrCreateFor(lhs, context_);
        auto lhs_pli_pointer =
                std::holds_alternative<model::PositionListIndex*>(lhs_pli)
                        ? std::get<model::PositionListIndex*>(lhs_pli)
                        : std::get<std::unique_ptr<model::PositionListIndex>>(lhs_pli).get();
        auto joint_pli = context_->GetPliCache()->Get(lhs.Union(static_cast<Vertical>(*rhs_)));
        error = joint_pli == nullptr
                        ? CalculateG1(lhs_pli_pointer)
                        : CalculateG1(lhs_pli_pointer->GetNepAsLong() - joint_pli->GetNepAsLong());
    }
    calc_count_++;
    return error;
}

model::ConfidenceInterval FdG1Strategy::CalculateG1(
        model::ConfidenceInterval const& num_violations) const {
    return model::ConfidenceInterval(CalculateG1(num_violations.GetMin()),
                                     CalculateG1(num_violations.GetMean()),
                                     CalculateG1(num_violations.GetMax()));
}

DependencyCandidate FdG1Strategy::CreateDependencyCandidate(Vertical const& vertical) const {
    if (context_->IsAgreeSetSamplesEmpty()) {
        return DependencyCandidate(vertical, model::ConfidenceInterval(0, .5, 1), false);
    }

    auto agree_set_sample = context_->GetAgreeSetSample(vertical);
    model::ConfidenceInterval num_violating_tuple_pairs =
            agree_set_sample
                    ->EstimateMixed(vertical, static_cast<Vertical>(*rhs_),
                                    context_->GetParameters().estimate_confidence)
                    .Multiply(context_->GetColumnLayoutRelationData()->GetNumTuplePairs());
    model::ConfidenceInterval g1 = CalculateG1(num_violating_tuple_pairs);
    return DependencyCandidate(vertical, g1, false);
}

void FdG1Strategy::RegisterDependency(Vertical const& vertical, double error,
                                      DependencyConsumer const& discovery_unit) const {
    discovery_unit.RegisterFd(vertical, *rhs_, error, 0);  // TODO: calculate score?
}

std::unique_ptr<DependencyStrategy> FdG1Strategy::CreateClone() {
    return std::make_unique<FdG1Strategy>(rhs_,
                                          (max_dependency_error_ + min_non_dependency_error_) / 2,
                                          (max_dependency_error_ - min_non_dependency_error_) / 2);
}
