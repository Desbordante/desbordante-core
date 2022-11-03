#include "sampler.h"

#include <utility>

#include "efficiency.h"
#include "util/pli_util.h"

namespace {

class ClusterComparator {
private:
    algos::hyfd::RowsPtr sort_keys_;
    size_t active_key_1_;
    size_t active_key_2_;

    size_t Increment(size_t number) noexcept {
        return (number == (*sort_keys_)[0].size() - 1) ? 0 : number + 1;
    }

public:
    ClusterComparator(algos::hyfd::RowsPtr sort_keys, size_t active_key_1) noexcept
        : sort_keys_(std::move(sort_keys)), active_key_1_(active_key_1), active_key_2_(1) {
        assert((*sort_keys_)[0].size() >= 3);
    }

    void IncrementActiveKey() noexcept {
        active_key_1_ = Increment(active_key_1_);
        active_key_2_ = Increment(active_key_2_);
    }

    bool operator()(size_t o1, size_t o2) noexcept {
        size_t value1 = (*sort_keys_)[o1][active_key_1_];
        size_t value2 = (*sort_keys_)[o2][active_key_1_];
        if (value1 == value2) {
            value1 = (*sort_keys_)[o1][active_key_2_];
            value2 = (*sort_keys_)[o2][active_key_2_];
        }
        return value1 > value2;
    }
};

}  // namespace

namespace algos::hyfd {

void Sampler::RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli) {
    efficiency.IncrementWindow();

    size_t const num_attributes = non_fds_->NumAttributes();
    size_t const prev_num_non_fds = non_fds_->Count();

    unsigned comparisons = 0;
    unsigned const window = efficiency.GetWindow();

    for (auto const& cluster : pli.GetIndex()) {
        for (size_t i = 0; window < cluster.size() && i < cluster.size() - window; ++i) {
            int const pivotId = cluster[i];
            int const partnerId = cluster[i + window];

            boost::dynamic_bitset<> equalAttrs(num_attributes);
            Match(equalAttrs, pivotId, partnerId);
            non_fds_->Add(std::move(equalAttrs));

            comparisons++;
        }
    }

    size_t const num_new_violations = non_fds_->Count() - prev_num_non_fds;

    efficiency.SetViolations(num_new_violations);
    efficiency.SetComparisons(comparisons);
}

void Sampler::ProcessComparisonSuggestions(IdPairs const& comparison_suggestions) {
    size_t const num_attributes = plis_.size();

    for (auto [first_id, second_id] : comparison_suggestions) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        Match(equal_attrs, first_id, second_id);

        non_fds_->Add(std::move(equal_attrs));
    }
}

void Sampler::InitializeEfficiencyQueue() {
    size_t const num_attributes = plis_.size();

    if (num_attributes >= 3) {
        ClusterComparator cluster_comparator(compressed_records_, (*compressed_records_)[0].size() - 1);
        for (auto& pli : plis_) {
            for (auto& cluster : pli->GetIndex()) {
                std::sort(cluster.begin(), cluster.end(), cluster_comparator);
            }
            cluster_comparator.IncrementActiveKey();
        }
    }

    for (size_t attr = 0; attr < num_attributes; ++attr) {
        Efficiency efficiency(attr);

        RunWindow(efficiency, *plis_[attr]);

        if (efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(efficiency);
        }
    }
    if (!efficiency_queue_.empty()) {
        efficiency_threshold_ = std::min(0.01, efficiency_queue_.top().CalcEfficiency() * 0.5);
    }
}

NonFDList Sampler::GetNonFDCandidates(IdPairs const& comparison_suggestions) {
    ProcessComparisonSuggestions(comparison_suggestions);

    if (efficiency_queue_.empty()) {
        InitializeEfficiencyQueue();
    } else {
        efficiency_threshold_ =
                std::min(efficiency_threshold_ / 2, efficiency_queue_.top().CalcEfficiency() * 0.9);
    }

    while (!efficiency_queue_.empty() &&
           efficiency_queue_.top().CalcEfficiency() >= efficiency_threshold_) {
        Efficiency best_efficiency = efficiency_queue_.top();
        efficiency_queue_.pop();

        RunWindow(best_efficiency, *plis_[best_efficiency.GetAttr()]);

        if (best_efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(best_efficiency);
        }
    }

    return non_fds_->MoveOutNewNonFds();
}

void Sampler::Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
                    size_t second_record_id) {
    assert(first_record_id < compressed_records_->size() &&
           second_record_id < compressed_records_->size());

    for (size_t i = 0; i < (*compressed_records_)[0].size(); ++i) {
        size_t const val1 = (*compressed_records_)[first_record_id][i];
        size_t const val2 = (*compressed_records_)[second_record_id][i];
        if (!PLIUtil::IsSingletonCluster(val1) && !PLIUtil::IsSingletonCluster(val2) &&
            val1 == val2) {
            attributes.set(i);
        }
    }
}

Sampler::Sampler(PLIs const& plis, RowsPtr pli_records)
    : plis_(plis), compressed_records_(std::move(pli_records)) {
    non_fds_ = std::make_shared<NonFds>(plis.size());
}

Sampler::~Sampler() = default;

}  // namespace algos::hyfd