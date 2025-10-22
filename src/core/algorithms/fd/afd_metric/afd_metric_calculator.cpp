#include "afd_metric_calculator.h"

#include <chrono>
#include <cmath>
#include <iterator>

#include <easylogging++.h>

#include "config/descriptions.h"
#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::afd_metric_calculator {

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
    auto start_time = std::chrono::system_clock::now();

    CalculateMetric();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
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
            result_ = CalculateTau(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::mu_plus:
            result_ = CalculateMuPlus(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::fi:
            result_ = CalculateFI(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
    }
}

long double AFDMetricCalculator::CalculateG2(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                             size_t num_rows) {
    assert(num_rows > 0);
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

std::pair<long double, long double> AFDMetricCalculator::CalculateP1P2(
        size_t num_rows, std::deque<model::PositionListIndex::Cluster>&& lhs_clusters,
        std::deque<model::PositionListIndex::Cluster>&& rhs_clusters) {
    assert(num_rows > 0);

    auto p1 = 0.L;
    for (auto& y : rhs_clusters) {
        auto size = y.size();
        p1 += size * size;
        std::sort(y.begin(), y.end());
    }
    p1 /= num_rows * num_rows;

    auto p2 = 0.L;
    for (auto& x : lhs_clusters) {
        std::sort(x.begin(), x.end());
        for (auto const& y : rhs_clusters) {
            model::PositionListIndex::Cluster xy;
            std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(xy));

            auto size = (long double)xy.size();
            if (size == 0.L) continue;
            p2 += size * size / x.size();
        }
    }
    p2 /= num_rows;

    return std::make_pair(p1, p2);
}

long double AFDMetricCalculator::CalculateTau(model::PLIWS const* lhs_pli,
                                              model::PLIWS const* rhs_pli, size_t num_rows) {
    assert(num_rows > 0);

    if (rhs_pli->GetNumCluster() < 2) {
        return 0.L;
    }

    auto [p1, p2] = CalculateP1P2(num_rows, lhs_pli->GetAllClusters(), rhs_pli->GetAllClusters());

    return (p2 - p1) / (1 - p1);
}

long double AFDMetricCalculator::CalculateMuPlus(model::PLIWS const* lhs_pli,
                                                 model::PLIWS const* rhs_pli, size_t num_rows) {
    assert(num_rows > 0);

    if (rhs_pli->GetNumCluster() < 2) {
        return 0.L;
    }

    auto lhs_clusters = lhs_pli->GetAllClusters();
    auto x_domain = lhs_clusters.size();
    if (num_rows == x_domain) {
        return 0.L;
    }

    auto [p1, p2] = CalculateP1P2(num_rows, std::move(lhs_clusters), rhs_pli->GetAllClusters());

    long double mu = 1 - (1 - p2) / (1 - p1) * (num_rows - 1) / (num_rows - x_domain);

    return std::max(mu, 0.L);
}

long double AFDMetricCalculator::CalculateFI(model::PLIWS const* lhs_pli,
                                             model::PLIWS const* rhs_pli, size_t num_rows) {
    assert(num_rows > 0);

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
