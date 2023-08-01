#include "sampler.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/future.hpp>

#include "algorithms/fd/hycommon/util/pli_util.h"
#include "efficiency.h"

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

template <typename F>
void Sampler::RunWindowImpl(Efficiency& efficiency, model::PositionListIndex const& pli,
                            F store_match) {
    efficiency.IncrementWindow();

    size_t const num_attributes = agree_sets_->NumAttributes();
    size_t const prev_num_agree_sets = agree_sets_->Count();

    unsigned comparisons = 0;
    unsigned const window = efficiency.GetWindow();

    for (model::PLI::Cluster const& cluster : pli.GetIndex()) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        for (size_t i = 0; window < cluster.size() && i < cluster.size() - window; ++i) {
            int const pivot_id = cluster[i];
            int const partner_id = cluster[i + window];

            Match(equal_attrs, pivot_id, partner_id);
            assert(equal_attrs.any());
            store_match(equal_attrs);
            equal_attrs.reset();

            comparisons++;
        }
    }

    size_t const num_new_violations = agree_sets_->Count() - prev_num_agree_sets;

    efficiency.SetViolations(num_new_violations);
    efficiency.SetComparisons(comparisons);
}

std::vector<boost::dynamic_bitset<>> Sampler::RunWindowRet(Efficiency& efficiency,
                                                           model::PositionListIndex const& pli) {
    std::vector<boost::dynamic_bitset<>> matched;
    auto store_match = [&matched](boost::dynamic_bitset<> const& equal_attrs) {
        matched.push_back(equal_attrs);
    };
    RunWindowImpl(efficiency, pli, store_match);
    return matched;
}

void Sampler::RunWindow(Efficiency& efficiency, model::PositionListIndex const& pli) {
    auto store_match = [this](boost::dynamic_bitset<> const& equal_attrs) {
        agree_sets_->Add(equal_attrs);
    };
    RunWindowImpl(efficiency, pli, store_match);
}

void Sampler::ProcessComparisonSuggestions(IdPairs const& comparison_suggestions) {
    size_t const num_attributes = plis_->size();

    for (auto [first_id, second_id] : comparison_suggestions) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        Match(equal_attrs, first_id, second_id);

        agree_sets_->Add(std::move(equal_attrs));
    }
}

void Sampler::SortClustersParallel() {
    ColumnSlider column_slider(plis_->size());
    std::vector<boost::unique_future<void>> sort_futures;
    for (model::PLI* pli : *plis_) {
        ClusterComparator cluster_comparator(compressed_records_.get(),
                                             column_slider.GetLeftNeighbor(),
                                             column_slider.GetRightNeighbor());
        auto sort = [pli, cluster_comparator]() {
            for (model::PLI::Cluster& cluster : pli->GetIndex()) {
                std::sort(cluster.begin(), cluster.end(), cluster_comparator);
            }
        };
        boost::packaged_task<void> task(std::move(sort));
        sort_futures.push_back(task.get_future());
        boost::asio::post(*pool_, std::move(task));
        column_slider.ToNextColumn();
    }
    boost::wait_for_all(sort_futures.begin(), sort_futures.end());
}

void Sampler::SortClustersSeq() {
    ColumnSlider column_slider(plis_->size());
    for (model::PLI* pli : *plis_) {
        ClusterComparator cluster_comparator(compressed_records_.get(),
                                             column_slider.GetLeftNeighbor(),
                                             column_slider.GetRightNeighbor());
        for (model::PLI::Cluster& cluster : pli->GetIndex()) {
            std::sort(cluster.begin(), cluster.end(), cluster_comparator);
        }
        column_slider.ToNextColumn();
    }
}

void Sampler::SortClusters() {
    if (threads_num_ > 1) {
        SortClustersParallel();
    } else {
        assert(threads_num_ == 1);
        SortClustersSeq();
    }
}

void Sampler::InitializeEfficiencyQueueParallel() {
    using EfficiencyAndMatches = std::pair<Efficiency, std::vector<boost::dynamic_bitset<>>>;
    std::vector<boost::unique_future<EfficiencyAndMatches>> futures;
    for (size_t attr = 0; attr < plis_->size(); ++attr) {
        auto run_window = [attr, this]() {
            Efficiency efficiency(attr);
            return std::make_pair(efficiency, RunWindowRet(efficiency, *(*plis_)[attr]));
        };
        boost::packaged_task<EfficiencyAndMatches> task(std::move(run_window));
        futures.push_back(task.get_future());
        boost::asio::post(*pool_, std::move(task));
    }

    // TODO(polyntsov): this waiting causes significant overhead on some datasets (on flight_1k
    // it's probably the highest one). However removing this waiting fully removes overhead on
    // these problematic datasets, it adds overhead on all other datasets :/
    // It seems that all problematic datasets spend most of the time on the validation phase, so
    // multithreading here may add some overhead, but I don't really understand how the explicit
    // waiting causes it. Further investigation is needed.
    boost::wait_for_all(futures.begin(), futures.end());

    for (auto& future : futures) {
        auto [efficiency, matches] = future.get();

        for (auto& match : matches) {
            agree_sets_->Add(std::move(match));
        }

        if (efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(efficiency);
        }
    }
}

void Sampler::InitializeEfficiencyQueueSeq() {
    for (size_t attr = 0; attr < plis_->size(); ++attr) {
        Efficiency efficiency(attr);
        RunWindow(efficiency, *(*plis_)[attr]);

        if (efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(efficiency);
        }
    }
}

void Sampler::InitializeEfficiencyQueueImpl() {
    if (threads_num_ > 1) {
        InitializeEfficiencyQueueParallel();
    } else {
        InitializeEfficiencyQueueSeq();
    }
}

void Sampler::InitializeEfficiencyQueue() {
    size_t const num_attributes = plis_->size();

    if (num_attributes >= 3) {
        SortClusters();
    }

    InitializeEfficiencyQueueImpl();

    if (!efficiency_queue_.empty()) {
        efficiency_threshold_ =
                std::min(kEfficiencyThreshold, efficiency_queue_.top().CalcEfficiency() / 2);
    }
}

ColumnCombinationList Sampler::GetAgreeSets(IdPairs const& comparison_suggestions) {
    ProcessComparisonSuggestions(comparison_suggestions);

    if (efficiency_queue_.empty()) {
        if (threads_num_ > 1) {
            assert(pool_ == nullptr);
            pool_ = std::make_unique<boost::asio::thread_pool>(threads_num_);
        }

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

Sampler::Sampler(PLIsPtr plis, RowsPtr pli_records, config::ThreadNumType threads)
    : plis_(std::move(plis)),
      compressed_records_(std::move(pli_records)),
      agree_sets_(std::make_unique<AllColumnCombinations>(plis_->size())),
      threads_num_(threads) {}

Sampler::~Sampler() {
    assert(threads_num_ != 0);
    if (threads_num_ != 1 && pool_ != nullptr) {
        pool_->join();
    }
}

}  // namespace algos::hy
