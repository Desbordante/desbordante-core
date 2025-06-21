#include "sampler.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/future.hpp>

namespace algos::dynfd {

namespace {

class ClusterComparator {
private:
    CompressedRecords const* sort_keys_;
    size_t comparison_column_1_;
    size_t comparison_column_2_;

public:
    ClusterComparator(CompressedRecords const* sort_keys, size_t comparison_column_1,
                      size_t comparison_column_2, DynamicRelationData const& relation) noexcept
        : sort_keys_(sort_keys),
          comparison_column_1_(comparison_column_1),
          comparison_column_2_(comparison_column_2) {
        assert(sort_keys_->front().size() >= 3);
        assert(comparison_column_1_ < relation.GetNumColumns());
        assert(comparison_column_2_ < relation.GetNumColumns());
        if (relation.GetColumnData(comparison_column_1_).GetPositionListIndex().GetClustersNum() <
            relation.GetColumnData(comparison_column_2_).GetPositionListIndex().GetClustersNum()) {
            std::swap(comparison_column_1_, comparison_column_2_);
        }
    }

    bool operator()(size_t o1, size_t o2) noexcept {
        int value1 = (*sort_keys_)[o1][comparison_column_1_];
        int value2 = (*sort_keys_)[o2][comparison_column_1_];
        if ((value1 < 0 && value2 < 0) || value1 == value2) {
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

template <typename F>
void Sampler::RunWindowImpl(algos::hy::Efficiency& efficiency, PLI const& pli, F store_match) {
    efficiency.IncrementWindow();

    size_t const num_attributes = relation_->GetNumColumns();
    size_t const prev_num_agree_sets = agree_sets_->Count();

    unsigned comparisons = 0;
    unsigned const window = efficiency.GetWindow();

    for (DPLI::Cluster const& cluster : pli) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        for (size_t i = 0; window < cluster.Size() && i < cluster.Size() - window; ++i) {
            int const pivot_id = *(cluster.begin() + i);
            int const partner_id = *(cluster.begin() + i + window);

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

std::vector<boost::dynamic_bitset<>> Sampler::RunWindowRet(algos::hy::Efficiency& efficiency,
                                                           PLI const& pli) {
    std::vector<boost::dynamic_bitset<>> matched;
    auto store_match = [&matched](boost::dynamic_bitset<> const& equal_attrs) {
        matched.push_back(equal_attrs);
    };
    RunWindowImpl(efficiency, pli, store_match);
    return matched;
}

void Sampler::RunWindow(algos::hy::Efficiency& efficiency, PLI const& pli) {
    auto store_match = [this](boost::dynamic_bitset<> const& equal_attrs) {
        agree_sets_->Add(equal_attrs);
    };
    RunWindowImpl(efficiency, pli, store_match);
}

void Sampler::ProcessComparisonSuggestions(IdPairs const& comparison_suggestions) {
    size_t const num_attributes = relation_->GetNumColumns();

    for (auto [first_id, second_id] : comparison_suggestions) {
        boost::dynamic_bitset<> equal_attrs(num_attributes);
        Match(equal_attrs, first_id, second_id);

        agree_sets_->Add(std::move(equal_attrs));
    }
}

void Sampler::SortClustersParallel() {
    ColumnSlider column_slider(relation_->GetNumColumns());
    std::vector<boost::unique_future<void>> sort_futures;
    for (auto& pli : plis_) {
        ClusterComparator cluster_comparator(compressed_records_.get(),
                                             column_slider.GetLeftNeighbor(),
                                             column_slider.GetRightNeighbor(), *relation_);
        auto sort = [pli, cluster_comparator]() mutable {
            for (DPLI::Cluster& cluster : pli) {
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
    ColumnSlider column_slider(relation_->GetNumColumns());
    for (auto& pli : plis_) {
        ClusterComparator cluster_comparator(compressed_records_.get(),
                                             column_slider.GetLeftNeighbor(),
                                             column_slider.GetRightNeighbor(), *relation_);
        for (DPLI::Cluster& cluster : pli) {
            std::sort(cluster.begin(), cluster.end(), cluster_comparator);
        }
        column_slider.ToNextColumn();
    }
}

void Sampler::SortClusters() {
    if (pool_) {
        SortClustersParallel();
    } else {
        SortClustersSeq();
    }
}

void Sampler::InitializeEfficiencyQueueParallel() {
    using EfficiencyAndMatches =
            std::pair<algos::hy::Efficiency, std::vector<boost::dynamic_bitset<>>>;
    std::vector<boost::unique_future<EfficiencyAndMatches>> futures;
    for (size_t attr = 0; attr < relation_->GetNumColumns(); ++attr) {
        auto run_window = [attr, this]() {
            algos::hy::Efficiency efficiency(attr);
            return std::make_pair(efficiency, RunWindowRet(efficiency, plis_[attr]));
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
    for (size_t attr = 0; attr < relation_->GetNumColumns(); ++attr) {
        algos::hy::Efficiency efficiency(attr);
        RunWindow(efficiency, plis_[attr]);

        if (efficiency.CalcEfficiency() > 0) {
            efficiency_queue_.push(efficiency);
        }
    }
}

void Sampler::InitializeEfficiencyQueueImpl() {
    if (pool_) {
        InitializeEfficiencyQueueParallel();
    } else {
        InitializeEfficiencyQueueSeq();
    }
}

void Sampler::InitializeEfficiencyQueue() {
    size_t const num_attributes = relation_->GetNumColumns();

    if (num_attributes >= 3) {
        SortClusters();
    }

    InitializeEfficiencyQueueImpl();
}

algos::hy::ColumnCombinationList Sampler::GetAgreeSets(IdPairs const& comparison_suggestions) {
    ProcessComparisonSuggestions(comparison_suggestions);

    if (efficiency_queue_.empty()) {
        for (size_t bit = 0; bit < relation_->GetNumColumns(); ++bit) {
            auto clusters_iterator =
                    relation_->GetColumnData(bit).GetPositionListIndex().GetClustersToCheck(
                            first_insert_batch_id_);
            plis_.emplace_back(clusters_iterator.begin(), clusters_iterator.end());
        }

        InitializeEfficiencyQueue();
    } else {
        efficiency_threshold_ = efficiency_threshold_ / 2;
    }

    while (!efficiency_queue_.empty() &&
           efficiency_queue_.top().CalcEfficiency() >= efficiency_threshold_) {
        algos::hy::Efficiency best_efficiency = efficiency_queue_.top();
        efficiency_queue_.pop();

        RunWindow(best_efficiency, plis_[best_efficiency.GetAttr()]);

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

    for (size_t i = 0; i < relation_->GetNumColumns(); ++i) {
        int val1 = (*compressed_records_)[first_record_id][i];
        int val2 = (*compressed_records_)[second_record_id][i];
        if (val1 >= 0 && val2 >= 0 && val1 == val2) {
            attributes.set(i);
        }
    }
}

Sampler::Sampler(std::shared_ptr<DynamicRelationData> relation, size_t first_insert_batch_id,
                 boost::asio::thread_pool* pool)
    : relation_(std::move(relation)), pool_(pool), first_insert_batch_id_(first_insert_batch_id) {
    compressed_records_ = relation_->GetCompressedRecordsPtr();
    agree_sets_ = std::make_unique<algos::hy::AllColumnCombinations>(relation_->GetNumColumns());
}

}  // namespace algos::dynfd
