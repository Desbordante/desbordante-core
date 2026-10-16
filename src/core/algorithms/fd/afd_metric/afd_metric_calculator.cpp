#include "afd_metric_calculator.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <stdexcept>
#include <unordered_map>

#include "core/config/descriptions.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/error/type.h"
#include "core/config/indices/option.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos::afd_metric_calculator {

using Cluster = model::PositionListIndex::Cluster;

namespace {

using ClusterSizeCounts = std::map<std::size_t, std::size_t>;

ClusterSizeCounts GetClusterSizeCounts(model::PLI const* pli) {
    ClusterSizeCounts size_counts;
    for (Cluster const& cluster : pli->GetIndex()) {
        ++size_counts[cluster.size()];
    }

    std::size_t const singleton_count = pli->GetRelationSize() - pli->GetSize();
    if (singleton_count != 0) size_counts[1] = singleton_count;
    return size_counts;
}

long double ExpectedMutualInformationCell(std::size_t num_rows, std::size_t lhs_size,
                                          std::size_t rhs_size) {
    // max(0, lhs_size + rhs_size - num_rows), written without unsigned underflow.
    std::size_t const rows_outside_rhs = num_rows - rhs_size;
    std::size_t const min_intersection =
            lhs_size > rows_outside_rhs ? lhs_size - rows_outside_rhs : 0;
    std::size_t const max_intersection = std::min(lhs_size, rhs_size);
    std::size_t mode = static_cast<std::size_t>((static_cast<long double>(lhs_size) + 1) *
                                                (static_cast<long double>(rhs_size) + 1) /
                                                (static_cast<long double>(num_rows) + 2));
    mode = std::clamp(mode, min_intersection, max_intersection);

    auto lower_weight = [=](long double weight, std::size_t intersection) {
        return weight * intersection * (num_rows - rhs_size - lhs_size + intersection) /
               ((rhs_size - intersection + 1.L) * (lhs_size - intersection + 1.L));
    };
    auto upper_weight = [=](long double weight, std::size_t intersection) {
        return weight * (rhs_size - intersection) * (lhs_size - intersection) /
               ((intersection + 1.L) * (num_rows - rhs_size - lhs_size + intersection + 1.L));
    };
    auto mutual_information_term = [=](std::size_t intersection) {
        if (intersection == 0) return 0.L;
        long double const count = intersection;
        return count / num_rows *
               std::log(count * num_rows / (static_cast<long double>(lhs_size) * rhs_size));
    };

    // Relative weights centered at the hypergeometric mode avoid underflow.
    long double total_weight = 1.L;
    long double weighted_term = mutual_information_term(mode);
    long double weight = 1.L;
    for (std::size_t intersection = mode; intersection > min_intersection; --intersection) {
        weight = lower_weight(weight, intersection);
        total_weight += weight;
        weighted_term += weight * mutual_information_term(intersection - 1);
    }

    weight = 1.L;
    for (std::size_t intersection = mode; intersection < max_intersection; ++intersection) {
        weight = upper_weight(weight, intersection);
        total_weight += weight;
        weighted_term += weight * mutual_information_term(intersection + 1);
    }
    return weighted_term / total_weight;
}

long double CalculateExpectedMutualInformation(model::PLI const* lhs_pli,
                                               model::PLI const* rhs_pli) {
    std::size_t const num_rows = lhs_pli->GetRelationSize();
    ClusterSizeCounts const lhs_size_counts = GetClusterSizeCounts(lhs_pli);
    ClusterSizeCounts const rhs_size_counts = GetClusterSizeCounts(rhs_pli);

    long double expected_mutual_information = 0.L;
    for (auto const& [lhs_size, lhs_count] : lhs_size_counts) {
        for (auto const& [rhs_size, rhs_count] : rhs_size_counts) {
            expected_mutual_information +=
                    static_cast<long double>(lhs_count) * rhs_count *
                    ExpectedMutualInformationCell(num_rows, lhs_size, rhs_size);
        }
    }
    return expected_mutual_information;
}

long double CalculateConditionalEntropy(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                        std::size_t num_rows) {
    long double conditional_entropy = 0.L;
    for (Cluster const& x : lhs_pli->GetIndex()) {
        long double const x_size = x.size();
        long double const log_x = std::log(x_size);

        std::size_t non_singleton_records = 0;
        for (Cluster const& y : rhs_pli->GetIndex()) {
            model::PositionListIndex::Cluster xy;
            std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(xy));

            long double const size = xy.size();
            if (size == 0.L) continue;
            non_singleton_records += size;
            conditional_entropy -= size * (std::log(size) - log_x);
        }

        conditional_entropy += (x.size() - non_singleton_records) * log_x;
    }
    return conditional_entropy / num_rows;
}

bool IsExactFd(model::PLI const* lhs_pli, model::PLI const* rhs_pli) {
    // The paper defines every exact FD as measure 1. The general RFI formulas
    // may be below 1 or undefined for exact keys and constant RHSs.
    return AFDMetricCalculator::CalculateG2Error(lhs_pli, rhs_pli, lhs_pli->GetRelationSize()) ==
           0.L;
}

long double CalculateExpectedFi(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                long double rhs_entropy) {
    long double const expected_mutual_information =
            std::max(CalculateExpectedMutualInformation(lhs_pli, rhs_pli), 0.L);
    return expected_mutual_information / rhs_entropy;
}

}  // namespace

AFDMetricCalculator::AFDMetricCalculator() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void AFDMetricCalculator::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
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
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty() || relation_->GetNumRows() == 0) {
        throw std::runtime_error("Got an empty dataset: AFD metric calculation is meaningless.");
    }
}

void AFDMetricCalculator::ExecuteInternal() {
    auto num_rows = relation_->GetNumRows();
    auto lhs_pli = relation_->CalculatePLI(lhs_indices_);
    auto rhs_pli = relation_->CalculatePLI(rhs_indices_);

    switch (metric_) {
        case AFDMetric::kG2:
            result_ = 1 - CalculateG2Error(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::kTau:
            result_ = CalculateTau(lhs_pli.get(), rhs_pli.get());
            break;
        case AFDMetric::kMuPlus:
            result_ = CalculateMuPlus(lhs_pli.get(), rhs_pli.get());
            break;
        case AFDMetric::kFi:
            result_ = CalculateFI(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::kG1:
            result_ = 1 - CalculateG1Error(lhs_pli.get(), lhs_pli->Intersect(rhs_pli.get()).get(),
                                           relation_->GetNumTuplePairs());
            break;
        case AFDMetric::kG3:
            result_ = CalculateG3(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::kPdep:
            result_ = CalculatePdepMeasure(lhs_pli.get(), rhs_pli.get());
            break;
        case AFDMetric::kRho:
            result_ = CalculateRhoMeasure(lhs_pli.get(), lhs_pli->Intersect(rhs_pli.get()).get());
            break;
        case AFDMetric::kRfiPlus:
            result_ = CalculateRfiPlusMeasure(lhs_pli.get(), rhs_pli.get());
            break;
        case AFDMetric::kG1S:
            result_ = CalculateG1SMeasure(lhs_pli.get(), rhs_pli.get(), num_rows);
            break;
        case AFDMetric::kRfiPrimePlus:
            result_ = CalculateRfiPrimePlusMeasure(lhs_pli.get(), rhs_pli.get());
            break;
    }
}

long double AFDMetricCalculator::CalculateG2Error(model::PLI const* lhs_pli,
                                                  model::PLI const* rhs_pli, size_t num_rows) {
    if (num_rows <= 0) throw std::invalid_argument("received non-positive number of rows");

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

long double AFDMetricCalculator::CalculateG3(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                             std::size_t num_rows) {
    if (num_rows <= 0) throw std::invalid_argument("received non-positive number of rows");

    std::size_t const lhs_singleton_rows = lhs_pli->GetRelationSize() - lhs_pli->GetSize();
    std::size_t max_fd_holds_rows = lhs_singleton_rows;

    std::deque<model::PLI::Cluster> const& lhs_clusters = lhs_pli->GetIndex();
    std::shared_ptr<std::vector<int> const> probing_table_ptr =
            rhs_pli->CalculateAndGetProbingTable();
    std::vector<int> const& probing_table = *probing_table_ptr;
    for (model::PLI::Cluster const& cluster : lhs_clusters) {
        auto frequencies = model::PLI::CreateFrequencies(cluster, probing_table);
        std::size_t max_fd_holds_in_cluster = 1;
        for (auto const& [rhs_value, rhs_cluster_size] : frequencies) {
            if (rhs_cluster_size > max_fd_holds_in_cluster)
                max_fd_holds_in_cluster = rhs_cluster_size;
        }
        max_fd_holds_rows += max_fd_holds_in_cluster;
    }

    return max_fd_holds_rows / static_cast<long double>(num_rows);
}

config::ErrorType AFDMetricCalculator::CalculateRhoMeasure(model::PLI const* x_pli,
                                                           model::PLI const* xa_pli) {
    auto calculate_dom = [](model::PLI const* pli) {
        auto index = pli->GetIndex();
        size_t dom = index.size();

        std::size_t cluster_rows_count = 0;
        for (Cluster const& cluster : index) {
            cluster_rows_count += cluster.size();
        }

        std::size_t unique_rows = pli->GetRelationSize() - cluster_rows_count;
        dom += unique_rows;
        return static_cast<config::ErrorType>(dom);
    };

    config::ErrorType dom_x = calculate_dom(x_pli);
    config::ErrorType dom_xa = calculate_dom(xa_pli);
    return dom_x / dom_xa;
}

long double AFDMetricCalculator::CalculatePdepSelf(model::PLI const* x_pli) {
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

long double AFDMetricCalculator::CalculatePdepMeasure(model::PLI const* x_pli,
                                                      model::PLI const* a_pli) {
    size_t n = x_pli->GetRelationSize();
    config::ErrorType sum = n - x_pli->GetSize();
    std::shared_ptr<std::vector<int> const> const a_prob = a_pli->CalculateAndGetProbingTable();

    for (Cluster const& x_cluster : x_pli->GetIndex()) {
        config::ErrorType x_cluster_size = x_cluster.size();
        std::unordered_map<int, unsigned> frequencies;
        size_t singleton_records = 0;

        for (int tuple_index : x_cluster) {
            int const value_id = (*a_prob)[tuple_index];
            if (value_id == model::PLI::kSingletonValueId) {
                ++singleton_records;
            } else {
                ++frequencies[value_id];
            }
        }

        for (auto const& [_, count] : frequencies) {
            sum += static_cast<config::ErrorType>(count) * count / x_cluster_size;
        }

        // Singleton A values represent distinct joint clusters of size one.
        sum += singleton_records / x_cluster_size;
    }

    return sum / n;
}

long double AFDMetricCalculator::CalculateTau(model::PLI const* lhs_pli,
                                              model::PLI const* rhs_pli) {
    auto p1 = CalculatePdepSelf(rhs_pli);
    if (p1 == 1) return 1;

    auto p2 = CalculatePdepMeasure(lhs_pli, rhs_pli);

    return (p2 - p1) / (1 - p1);
}

long double AFDMetricCalculator::CalculateMuPlus(model::PLI const* x_pli, model::PLI const* a_pli) {
    config::ErrorType pdep_y = CalculatePdepSelf(a_pli);
    if (pdep_y == 1) return 1;

    config::ErrorType pdep_xy = CalculatePdepMeasure(x_pli, a_pli);

    size_t n = x_pli->GetRelationSize();
    size_t k = x_pli->GetNumCluster();

    if (k == n) return 1;

    config::ErrorType mu = 1 - (1 - pdep_xy) / (1 - pdep_y) * (n - 1) / (n - k);
    return std::max(0., mu);
}

long double AFDMetricCalculator::CalculateFI(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                             size_t num_rows) {
    if (num_rows <= 0) throw std::invalid_argument("received non-positive number of rows");

    if (rhs_pli->GetNumCluster() < 2) {
        return 0.L;
    }

    long double const entropy = rhs_pli->GetEntropy();
    long double const conditional_entropy = CalculateConditionalEntropy(lhs_pli, rhs_pli, num_rows);
    return (entropy - conditional_entropy) / entropy;
}

long double AFDMetricCalculator::CalculateG1SMeasure(model::PLI const* lhs_pli,
                                                     model::PLI const* rhs_pli,
                                                     std::size_t num_rows) {
    if (num_rows == 0 || lhs_pli->GetRelationSize() != num_rows ||
        rhs_pli->GetRelationSize() != num_rows) {
        throw std::invalid_argument("received empty or incompatible PLIs");
    }

    long double const conditional_entropy = CalculateConditionalEntropy(lhs_pli, rhs_pli, num_rows);
    return std::max(1.L - conditional_entropy, 0.L);
}

long double AFDMetricCalculator::CalculateRfiPlusMeasure(model::PLI const* lhs_pli,
                                                         model::PLI const* rhs_pli) {
    std::size_t const num_rows = lhs_pli->GetRelationSize();
    if (num_rows == 0 || rhs_pli->GetRelationSize() != num_rows) {
        throw std::invalid_argument("received empty or incompatible PLIs");
    }

    // The paper defines every exact FD, including keys and constant RHSs, as one.
    if (IsExactFd(lhs_pli, rhs_pli)) return 1.L;

    long double const rhs_entropy = rhs_pli->GetEntropy();
    long double const observed_fi = CalculateFI(lhs_pli, rhs_pli, num_rows);
    long double const expected_fi = CalculateExpectedFi(lhs_pli, rhs_pli, rhs_entropy);
    return std::max(observed_fi - expected_fi, 0.L);
}

long double AFDMetricCalculator::CalculateRfiPrimePlusMeasure(model::PLI const* lhs_pli,
                                                              model::PLI const* rhs_pli) {
    std::size_t const num_rows = lhs_pli->GetRelationSize();
    if (num_rows == 0 || rhs_pli->GetRelationSize() != num_rows) {
        throw std::invalid_argument("received empty or incompatible PLIs");
    }

    if (IsExactFd(lhs_pli, rhs_pli)) return 1.L;

    long double const rhs_entropy = rhs_pli->GetEntropy();
    long double const observed_fi = CalculateFI(lhs_pli, rhs_pli, num_rows);
    long double const expected_fi = CalculateExpectedFi(lhs_pli, rhs_pli, rhs_entropy);
    long double const normalization = 1.L - expected_fi;
    if (normalization <= 0.L) return 0.L;
    return std::clamp((observed_fi - expected_fi) / normalization, 0.L, 1.L);
}

config::ErrorType AFDMetricCalculator::CalculateZeroAryG1(ColumnData const* rhs,
                                                          unsigned long long num_tuple_pairs) {
    // "g1 zero-ary" variant for empty-LHS FDs (->A). TANE needs this to evaluate
    // constant-column candidates before building the lattice. It does not require
    // intersecting with an LHS PLI, unlike CalculateG1Error below.
    return 1 - rhs->GetPositionListIndex()->GetNepAsLong() /
                       static_cast<config::ErrorType>(num_tuple_pairs);
}

config::ErrorType AFDMetricCalculator::CalculateG1Error(model::PLI const* lhs_pli,
                                                        model::PLI const* joint_pli,
                                                        unsigned long long num_tuple_pairs) {
    // Non-standard g1: divides by num_tuple_pairs instead of (n choose 2). Kept
    // intentionally — Tane originally used this variant and all hash-based
    // regression tests were written against it. Fixing it changes those hashes.
    return static_cast<config::ErrorType>((lhs_pli->GetNepAsLong() - joint_pli->GetNepAsLong()) /
                                          static_cast<config::ErrorType>(num_tuple_pairs));
}

}  // namespace algos::afd_metric_calculator
