#include "afd_metric_calculator.h"

#include <chrono>
#include <cmath>
#include <iterator>

#include "util/timed_invoke.h"
#include "config/descriptions.h"
#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"


namespace algos::afd_metric_calculator {

using Cluster = model::PositionListIndex::Cluster;

AFDMetricCalculator::AFDMetricCalculator() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void AFDMetricCalculator::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_cols));
    RegisterOption(Option{&metric_, kMetric, kDAFDMetric});
}

void AFDMetricCalculator::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable(
            {kMetric, config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName()});
}

void AFDMetricCalculator::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: AFD metric calculation is meaningless.");
    }
}

unsigned long long AFDMetricCalculator::ExecuteInternal() {
    size_t const elapsed_milliseconds = util::TimedInvoke(&AFDMetricCalculator::CalculateMetric, this);

    return elapsed_milliseconds;
}

void AFDMetricCalculator::CalculateMetric() {
    auto num_rows = relation_->GetNumRows();
    auto lhs_pli = relation_->CalculatePLIWS(lhs_indices_);
    auto rhs_pli = relation_->CalculatePLIWS(rhs_indices_);

    switch (metric_) {
        case AFDMetric::g2:
            result_ = CalculateG2(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::tau:
            result_ = CalculateTau(lhs_pli.get(), rhs_pli.get(),
                                   lhs_pli->Intersect(rhs_pli.get()).get());
            break;
        case AFDMetric::mu_plus:
            result_ = CalculateMuPlus(lhs_pli.get(), rhs_pli.get(),
                                      lhs_pli->Intersect(rhs_pli.get()).get());
            break;
        case AFDMetric::fi:
            result_ = CalculateFI(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
    }
}

long double AFDMetricCalculator::CalculateG2(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                             size_t num_rows) {
    if(num_rows <= 0)
        throw std::invalid_argument("received unpositive number of rows");

    auto num_error_rows = 0.L;

    auto const& lhs_clusters = lhs_pli->GetIndex();
    auto pt_shared = rhs_pli->CalculateAndGetProbingTable();
    auto const& pt = *pt_shared.get();
    for (auto const& cluster : lhs_clusters) {
        auto frequencies = model::PLI::CreateFrequencies(cluster, pt);
        auto size = cluster.size();
        if (frequencies.size() != 1 || frequencies.begin()->second != size) num_error_rows += size;
    }

    return num_error_rows / num_rows;
}

long double AFDMetricCalculator::CalculatePdepSelf(model::PLIWithSingletons const* x_pli) {
    size_t n = x_pli->GetRelationSize();
    config::ErrorType sum = 0;
    std::size_t cluster_rows_count = 0;
    std::deque<Cluster> const& x_index = x_pli->GetIndex();
    for (Cluster const& x_cluster : x_index) {
        cluster_rows_count += x_cluster.size();
        sum += x_cluster.size() * x_cluster.size();
    }
    std::size_t unique_rows = x_pli->GetRelationSize() - cluster_rows_count;
    sum += unique_rows;
    return static_cast<config::ErrorType>(sum / (n * n));
}

long double AFDMetricCalculator::CalculatePdepMeasure(model::PLIWithSingletons const* x_pli,
                                                      model::PLIWithSingletons const* xa_pli) {
    std::deque<Cluster> xa_index = xa_pli->GetIndex();
    std::deque<Cluster> x_index = x_pli->GetIndex();
    size_t n = x_pli->GetRelationSize();

    config::ErrorType sum = 0;

    std::unordered_map<int, size_t> x_frequencies;

    int x_value_id = 1;
    for (Cluster const& x_cluster : x_index) {
        x_frequencies[x_value_id++] = x_cluster.size();
    }

    x_frequencies[model::PositionListIndex::kSingletonValueId] = 1;

    auto x_prob = x_pli->CalculateAndGetProbingTable();

    auto get_x_freq_by_tuple_ind{[&x_prob, &x_frequencies](int tuple_ind) {
        int value_id = x_prob->at(tuple_ind);
        return static_cast<config::ErrorType>(x_frequencies[value_id]);
    }};

    for (Cluster const& xa_cluster : xa_index) {
        config::ErrorType num = xa_cluster.size() * xa_cluster.size();
        config::ErrorType denum = get_x_freq_by_tuple_ind(xa_cluster.front());
        sum += num / denum;
    }

    std::deque<Cluster> xa_sngt = xa_pli->GetSingletons();
    for (Cluster const& xa_cluster : xa_sngt) {
        for (auto const& el : xa_cluster) {
            config::ErrorType denum = get_x_freq_by_tuple_ind(el);
            sum += 1 / denum;
        }
    }

    return (sum / static_cast<config::ErrorType>(n));
}

long double AFDMetricCalculator::CalculateTau(model::PLIWS const* lhs_pli,
                                              model::PLIWS const* rhs_pli,
                                              model::PLIWS const* joint_pli) {
    auto p1 = CalculatePdepSelf(rhs_pli);
    if (p1 == 1) return 1;

    auto p2 = CalculatePdepMeasure(lhs_pli, joint_pli);

    return (p2 - p1) / (1 - p1);
}

long double AFDMetricCalculator::CalculateMuPlus(model::PLIWS const* lhs_pli,
                                                 model::PLIWS const* rhs_pli,
                                                 model::PLIWS const* joint_pli) {
    auto num_rows = rhs_pli->GetRelationSize();

    if (rhs_pli->GetNumCluster() < 2) {
        return 1;
    }

    auto lhs_clusters = lhs_pli->GetAllClusters();
    auto x_domain = lhs_clusters.size();
    if (num_rows == x_domain) {
        return 1;
    }

    auto p1 = CalculatePdepSelf(rhs_pli);
    auto p2 = CalculatePdepMeasure(lhs_pli, joint_pli);

    if (p1 == 1) return 1;

    long double mu = 1 - (1 - p2) / (1 - p1) * (num_rows - 1) / (num_rows - x_domain);

    return std::max(mu, 0.L);
}

long double AFDMetricCalculator::CalculateFI(model::PLIWS const* lhs_pli,
                                             model::PLIWS const* rhs_pli, size_t num_rows) {
    if(num_rows <= 0)
        throw std::invalid_argument("received unpositive number of rows");

    if (rhs_pli->GetNumCluster() < 2) {
        return 0.L;
    }

    auto entropy = rhs_pli->GetEntropy();

    auto rhs_clusters = rhs_pli->GetAllClusters();
    for (auto& y : rhs_clusters) {
        std::sort(y.begin(), y.end());
    }

    auto conditional_entropy = 0.L;
    for (auto& x : lhs_pli->GetAllClusters()) {
        std::sort(x.begin(), x.end());
        auto log_x = std::log(x.size());
        for (auto const& y : rhs_clusters) {
            model::PositionListIndex::Cluster xy;
            std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(xy));

            auto size = (long double)xy.size();
            if (size == 0.L) continue;
            conditional_entropy -= size * (std::log(size) - log_x);
        }
    }
    conditional_entropy /= num_rows;
    auto mutual_information = entropy - conditional_entropy;
    return mutual_information / entropy;
}

}  // namespace algos::afd_metric_calculator
