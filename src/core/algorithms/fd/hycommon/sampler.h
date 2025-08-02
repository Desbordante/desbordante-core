#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <algorithm>
#include <utility>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/future.hpp>

#include "core/algorithms/fd/hycommon/all_column_combinations.h"
#include "core/algorithms/fd/hycommon/efficiency.h"

namespace algos::hy {

template <typename Policy>
class Sampler {
private:
    friend Policy;
    Policy policy_;

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

    double efficiency_threshold_;

    Policy::PLIsPtr plis_;
    Policy::RowsPtr compressed_records_;
    std::priority_queue<Efficiency> efficiency_queue_;
    std::unique_ptr<AllColumnCombinations> agree_sets_;
    boost::asio::thread_pool* pool_;
    size_t num_attributes_;

    void ProcessComparisonSuggestions(Policy::IdPairs const& comparison_suggestions) {
        for (auto [first_id, second_id] : comparison_suggestions) {
            boost::dynamic_bitset<> equal_attrs(num_attributes_);
            Match(equal_attrs, first_id, second_id);
            agree_sets_->Add(std::move(equal_attrs));
        }
    }

    void SortClustersSeq() {
        ColumnSlider column_slider(num_attributes_);
        for (typename Policy::PLIPtr pli : *plis_) {
            typename Policy::ClusterComparator cluster_comparator(
                compressed_records_.get(),
                column_slider.GetLeftNeighbor(),
                column_slider.GetRightNeighbor(),
                policy_
            );
            for (typename Policy::Cluster& cluster : *pli) {
                std::sort(cluster.begin(), cluster.end(), cluster_comparator);
            }
            column_slider.ToNextColumn();
        }
    }

    void SortClustersParallel() {
        ColumnSlider column_slider(num_attributes_);
        std::vector<boost::unique_future<void>> sort_futures;
        for (typename Policy::PLIPtr pli : *plis_) {
            typename Policy::ClusterComparator cluster_comparator(
                compressed_records_.get(),
                column_slider.GetLeftNeighbor(),
                column_slider.GetRightNeighbor(),
                policy_
            );
            auto sort = [pli, cluster_comparator]() {
                for (typename Policy::Cluster& cluster : *pli) {
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

    void SortClusters() {
        if (pool_) {
            SortClustersParallel();
        } else {
            SortClustersSeq();
        }
    }

    void InitializeEfficiencyQueueSeq() {
        for (size_t attr = 0; attr < num_attributes_; ++attr) {
            Efficiency efficiency(attr);
            RunWindow(efficiency, *(*plis_)[attr]);

            if (efficiency.CalcEfficiency() > 0) {
                efficiency_queue_.push(efficiency);
            }
        }
    }

    void InitializeEfficiencyQueueParallel() {
        using EfficiencyAndMatches = std::pair<Efficiency, std::vector<boost::dynamic_bitset<>>>;
        std::vector<boost::unique_future<EfficiencyAndMatches>> futures;
        for (size_t attr = 0; attr < num_attributes_; ++attr) {
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

    void InitializeEfficiencyQueueImpl() {
        if (pool_) {
            InitializeEfficiencyQueueParallel();
        } else {
            InitializeEfficiencyQueueSeq();
        }
    }

    void InitializeEfficiencyQueue() {
        if (num_attributes_ >= 3) {
            SortClusters();
        }

        InitializeEfficiencyQueueImpl();

        if (!efficiency_queue_.empty()) {
            efficiency_threshold_ =
                    std::min(efficiency_threshold_, efficiency_queue_.top().CalcEfficiency() / 2);
        }
    }

    void Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
               size_t second_record_id) {
        assert(first_record_id < compressed_records_->size() &&
               second_record_id < compressed_records_->size());

        for (size_t i = 0; i < num_attributes_; ++i) {
            typename Policy::TablePos const val1 = (*compressed_records_)[first_record_id][i];
            typename Policy::TablePos const val2 = (*compressed_records_)[second_record_id][i];
            if (!policy_.IsSingletonCluster(val1) && !policy_.IsSingletonCluster(val2) &&
                val1 == val2) {
                attributes.set(i);
            }
        }
    }

    template <typename F>
    void RunWindowImpl(Efficiency& efficiency, Policy::PLI const& pli, F store_match) {
        efficiency.IncrementWindow();

        size_t const prev_num_agree_sets = agree_sets_->Count();

        unsigned comparisons = 0;
        unsigned const window = efficiency.GetWindow();

        for (typename Policy::Cluster const& cluster : pli) {
            boost::dynamic_bitset<> equal_attrs(num_attributes_);
            size_t cluster_size = static_cast<size_t>(cluster.end() - cluster.begin());
            for (size_t i = 0; window < cluster_size && i < cluster_size - window; ++i) {
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

    std::vector<boost::dynamic_bitset<>> RunWindowRet(Efficiency& efficiency,
                                                      Policy::PLI const& pli) {
        std::vector<boost::dynamic_bitset<>> matched;
        auto store_match = [&matched](boost::dynamic_bitset<> const& equal_attrs) {
            matched.push_back(equal_attrs);
        };
        RunWindowImpl(efficiency, pli, store_match);
        return matched;
    }

    void RunWindow(Efficiency& efficiency, Policy::PLI const& pli) {
        auto store_match = [this](boost::dynamic_bitset<> const& equal_attrs) {
            agree_sets_->Add(equal_attrs);
        };
        RunWindowImpl(efficiency, pli, store_match);
    }

public:
    Sampler(Policy::PLIsPtr plis,
            Policy::RowsPtr pli_records,
            double efficiency_threshold,
            boost::asio::thread_pool* pool,
            size_t num_attributes,
            Policy policy)
        : policy_(std::move(policy)),
          efficiency_threshold_(efficiency_threshold),
          plis_(std::move(plis)),
          compressed_records_(std::move(pli_records)),
          agree_sets_(std::make_unique<AllColumnCombinations>(num_attributes)),
          pool_(pool),
          num_attributes_(num_attributes) {};

    ColumnCombinationList GetAgreeSets(typename Policy::IdPairs const& comparison_suggestions) {
        ProcessComparisonSuggestions(comparison_suggestions);

        if (efficiency_queue_.empty()) {
            policy_.Initialize(*this);

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
};

}  // namespace algos::hy
