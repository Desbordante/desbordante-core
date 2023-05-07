#include "sampler.h"

#include <algorithm>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "efficiency.h"
#include "util/pli_util.h"

namespace {

class ClusterComparator {
private:
    algos::hy::Rows* sort_keys_;
    size_t comparison_column_1_;
    size_t comparison_column_2_;

public:
    ClusterComparator(algos::hy::Rows* sort_keys, size_t comparison_column_1,
                      size_t comparison_column_2) noexcept
        : sort_keys_(sort_keys),
          comparison_column_1_(comparison_column_1),
          comparison_column_2_(comparison_column_2) {
        assert(sort_keys_->front().size() >= 3);
    }

    bool operator()(size_t o1, size_t o2) noexcept {
        size_t value1 = (*sort_keys_)[o1][comparison_column_1_];
        size_t value2 = (*sort_keys_)[o2][comparison_column_1_];
        if (value1 == value2) {
            value1 = (*sort_keys_)[o1][comparison_column_2_];
            value2 = (*sort_keys_)[o2][comparison_column_2_];
        }
        return value1 > value2;
    }
};

class ColumnSlider {
private:
    size_t num_attributes_;
    size_t current_column_compared_ = 0;

    [[nodiscard]] size_t GetIncrement(size_t i) const {
        return (i == num_attributes_ - 1) ? 0 : i + 1;
    }

    [[nodiscard]] size_t GetDecrement(size_t i) const {
        return (i == 0) ? num_attributes_ - 1 : i - 1;
    }

public:
    explicit ColumnSlider(size_t num_attributes) : num_attributes_(num_attributes) {}

    void ToNextColumn() {
        current_column_compared_ = GetIncrement(current_column_compared_);
    }

    [[nodiscard]] size_t GetLeftNeighbor() const {
        return GetDecrement(current_column_compared_);
    }

    [[nodiscard]] size_t GetRightNeighbor() const {
        return GetIncrement(current_column_compared_);
    }
};

}  // namespace

namespace algos::hy {

void Sampler::RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli) {
    efficiency.IncrementWindow();

    size_t const num_attributes = agree_sets_->NumAttributes();
    size_t const prev_num_agree_sets = agree_sets_->Count();

    unsigned comparisons = 0;
    unsigned const window = efficiency.GetWindow();

    for (util::PLI::Cluster const& cluster : pli.GetIndex()) {
        for (size_t i = 0; window < cluster.size() && i < cluster.size() - window; ++i) {
            int const pivot_id = cluster[i];
            int const partner_id = cluster[i + window];

            boost::dynamic_bitset<> equal_attrs(num_attributes);
            Match(equal_attrs, pivot_id, partner_id);
            agree_sets_->Add(std::move(equal_attrs));

            comparisons++;
        }
    }

    size_t const num_new_violations = agree_sets_->Count() - prev_num_agree_sets;

    efficiency.SetViolations(num_new_violations);
    efficiency.SetComparisons(comparisons);
}

void Sampler::ProcessComparisonSuggestions(IdPairs const& comparison_suggestions) {
    size_t const num_attributes = plis_->size();

    for (auto [first_id, second_id] : comparison_suggestions) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        Match(equal_attrs, first_id, second_id);

        agree_sets_->Add(std::move(equal_attrs));
    }
}

void Sampler::InitializeEfficiencyQueue() {
    size_t const num_attributes = plis_->size();

    if (num_attributes >= 3) {
        ColumnSlider column_slider(num_attributes);
        for (util::PLI* pli : *plis_) {
            ClusterComparator cluster_comparator(compressed_records_.get(),
                                                 column_slider.GetLeftNeighbor(),
                                                 column_slider.GetRightNeighbor());
            for (util::PLI::Cluster& cluster : pli->GetIndex()) {
                std::sort(cluster.begin(), cluster.end(), cluster_comparator);
            }
            column_slider.ToNextColumn();
        }
    }

    for (size_t attr = 0; attr < num_attributes; ++attr) {
        Efficiency efficiency(attr);

        RunWindow(efficiency, *(*plis_)[attr]);

        if (efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(efficiency);
        }
    }
    if (!efficiency_queue_.empty()) {
        efficiency_threshold_ =
                std::min(kEfficiencyThreshold, efficiency_queue_.top().CalcEfficiency() / 2);
    }
}

ColumnCombinationList Sampler::GetAgreeSets(IdPairs const& comparison_suggestions) {
    ProcessComparisonSuggestions(comparison_suggestions);

    if (efficiency_queue_.empty()) {
        InitializeEfficiencyQueue();
    } else {
        double const threshold_decrease = 0.9;
        efficiency_threshold_ =
                std::min(efficiency_threshold_ / 2,
                         efficiency_queue_.top().CalcEfficiency() * threshold_decrease);
    }

    while (!efficiency_queue_.empty() &&
           efficiency_queue_.top().CalcEfficiency() >= efficiency_threshold_) {
        Efficiency best_efficiency = efficiency_queue_.top();
        efficiency_queue_.pop();

        RunWindow(best_efficiency, *(*plis_)[best_efficiency.GetAttr()]);

        if (best_efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(best_efficiency);
        }
    }

    return agree_sets_->MoveOutNewColumnCombinations();
}

void Sampler::Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
                    size_t second_record_id) {
    assert(first_record_id < compressed_records_->size() &&
           second_record_id < compressed_records_->size());

    for (size_t i = 0; i < compressed_records_->front().size(); ++i) {
        TablePos const val1 = (*compressed_records_)[first_record_id][i];
        TablePos const val2 = (*compressed_records_)[second_record_id][i];
        if (!PLIUtil::IsSingletonCluster(val1) && !PLIUtil::IsSingletonCluster(val2) &&
            val1 == val2) {
            attributes.set(i);
        }
    }
}

Sampler::Sampler(PLIsPtr plis, RowsPtr pli_records)
    : plis_(std::move(plis)),
      compressed_records_(std::move(pli_records)),
      agree_sets_(std::make_unique<AllColumnCombinations>(plis_->size())) {}

Sampler::~Sampler() = default;

}  // namespace algos::hy
