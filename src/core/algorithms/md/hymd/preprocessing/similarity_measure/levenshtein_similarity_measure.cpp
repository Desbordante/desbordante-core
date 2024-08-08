#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"

#include <atomic>
#include <cstddef>
#include <numeric>
#include <set>
#include <span>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/build_indexes.h"
#include "algorithms/md/hymd/preprocessing/encode_results.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "algorithms/md/hymd/utility/make_unique_for_overwrite.h"
#include "config/exceptions.h"
#include "model/types/double_type.h"
#include "model/types/string_type.h"

namespace {
using namespace algos::hymd;
using SimValPair = std::pair<preprocessing::Similarity, ValueIdentifier>;

// TODO: very messy, clean up.
std::size_t LevenshteinDistanceMain(unsigned* p, unsigned* d, std::size_t max_dist, auto* l,
                                    std::size_t l_size, auto* r, std::size_t r_size) {
    if (l_size <= max_dist) {
        std::iota(p, p + l_size + 1, 0);
    } else {
        auto iota_end = p + max_dist + 1;
        std::iota(p, iota_end, 0);
        std::fill(iota_end, p + l_size + 1, -1);
    }
    std::fill(d, d + l_size + 1, -1);
    DESBORDANTE_ASSUME(l_size <= r_size);
    DESBORDANTE_ASSUME(l_size != 0);
    model::Index i = 0;
    auto get_zero = [](model::Index) { return 0; };
    auto get_l_size = [l_size](model::Index) { return l_size; };
    auto get_n0_min = [max_dist](model::Index i) { return i - max_dist; };
    auto set_max = [](auto* d, model::Index min) { d[min] = -1; };
    auto ignore = [](auto*, model::Index) {};
    auto do_loop = [&r, &l](auto* p, auto* d, model::Index i, auto get_min, auto get_max,
                            auto max_func) {
        auto tj = r[i];
        *d = i + 1;

        model::Index const min = get_min(i);
        model::Index const max = get_max(i);

        max_func(d, min);
        for (model::Index j = min; j != max;) {
            unsigned value = p[j];
            if (l[j] == tj) {
                ++j;
            } else {
                unsigned const insert_cost = d[j];
                ++j;
                value = 1 + std::min({insert_cost, value, p[j]});
            }
            d[j] = value;
        }
    };
    if (r_size <= max_dist) {
        if (r_size % 2 == 0) {
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                ++i;
                do_loop(d, p, i, get_zero, get_l_size, ignore);
            }
            return p[l_size];
        } else {
            --r_size;
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                ++i;
                do_loop(d, p, i, get_zero, get_l_size, ignore);
            }
            do_loop(p, d, r_size, get_zero, get_l_size, ignore);
            return d[l_size];
        }
    } else {
        std::size_t const max_dist_inc = max_dist + 1;
        auto get_nl_max = [max_dist_inc](model::Index i) { return i + max_dist_inc; };
        DESBORDANTE_ASSUME(r_size - max_dist_inc < l_size)
        if (l_size <= max_dist_inc) {
            for (; i != max_dist_inc; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                std::swap(p, d);
            }
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                std::swap(p, d);
            }
        } else {
            if (l_size > 2 * max_dist_inc) {
                for (; i != max_dist_inc; ++i) {
                    do_loop(p, d, i, get_zero, get_nl_max, ignore);
                    std::swap(p, d);
                }
                for (; i != l_size - max_dist_inc; ++i) {
                    do_loop(p, d, i, get_n0_min, get_nl_max, set_max);
                    std::swap(p, d);
                }
                for (; i != r_size; ++i) {
                    do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                    std::swap(p, d);
                }
            } else {
                for (; i != l_size - (max_dist + 1); ++i) {
                    do_loop(p, d, i, get_zero, get_nl_max, ignore);
                    std::swap(p, d);
                }
                for (; i != max_dist + 1; ++i) {
                    do_loop(p, d, i, get_zero, get_l_size, ignore);
                    std::swap(p, d);
                }
                for (; i != r_size; ++i) {
                    do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                    std::swap(p, d);
                }
            }
        }
    }

    return p[l_size];
}

std::size_t LevenshteinDistance(std::string const* l_ptr, std::string const* r_ptr, unsigned* p,
                                unsigned* d, std::size_t max_dist,
                                std::size_t fail_value) noexcept {
    std::size_t r_size = r_ptr->size();
    std::size_t l_size = l_ptr->size();
    if (l_size > r_size) {
        std::swap(l_ptr, r_ptr);
        std::swap(l_size, r_size);
    }
    if (r_size - l_size > max_dist) return fail_value;

    auto* l = &l_ptr->front();
    auto* r = &r_ptr->front();

    while (l_size != 0 && l[l_size - 1] == r[r_size - 1]) {
        --r_size;
        --l_size;
    }

    while (l_size != 0 && *l == *r) {
        --r_size;
        --l_size;
        ++l;
        ++r;
    }

    if (l_size == 0) return r_size;

    return LevenshteinDistanceMain(p, d, max_dist, l, l_size, r, r_size);
}

using namespace algos::hymd::preprocessing;

std::size_t GetLargestStringSize(DataInfo const& data_info_left) {
    std::size_t const element_number = data_info_left.GetElementNumber();
    std::size_t max_size = 0;
    for (model::Index i = 0; i != element_number; ++i) {
        auto const& left_string = model::Type::GetValue<std::string>(data_info_left.GetAt(i));
        std::size_t const left_size = left_string.size();
        if (left_size > max_size) max_size = left_size;
    }
    return max_size;
}

class ValueProcessingWorker {
    using BufPtr = std::unique_ptr<unsigned[]>;

    struct Resource {
        BufPtr buf;
        bool dissimilar_found = false;
    };

    std::shared_ptr<DataInfo const> const& data_info_left_;
    std::shared_ptr<DataInfo const> const& data_info_right_;
    std::vector<indexes::PliCluster> const& clusters_right_;
    Similarity const min_sim_;
    std::size_t const data_left_size_ = data_info_left_->GetElementNumber();
    std::size_t const data_right_size_ = data_info_right_->GetElementNumber();
    ValidTableResults<Similarity> task_data_{data_left_size_};
    std::size_t const buf_len_ = GetLargestStringSize(*data_info_left_) + 1;

    void AddValue(RowInfo<Similarity>& row_info, ValueIdentifier value_id, Similarity sim) {
        auto& [sim_value_id_vec, valid_records_number] = row_info;
        sim_value_id_vec.emplace_back(sim, value_id);
        valid_records_number += clusters_right_[value_id].size();
    }

    Resource AcquireResource() const {
        // TODO: replace with std::make_unique_for_overwrite when GCC in CI is upgraded
        return {utility::MakeUniqueForOverwrite<unsigned[]>(buf_len_ * 2)};
    }

    void ProcessSame(ValueIdentifier const left_value_id, Resource& resource) {
        auto& [buf, dissimilar_found] = resource;
        RowInfo<Similarity>& row_info = task_data_[left_value_id];
        AddValue(row_info, left_value_id, 1.0);
        if (CalcAndAdd(left_value_id, buf, row_info, left_value_id + 1)) dissimilar_found = true;
    }

    void ProcessFull(ValueIdentifier const left_value_id, Resource& resource) {
        auto& [buf, dissimilar_found] = resource;
        if (CalcAndAdd(left_value_id, buf, task_data_[left_value_id], 0)) dissimilar_found = true;
    }

    bool CalcAndAdd(ValueIdentifier left_value_id, BufPtr const& buf, RowInfo<Similarity>& row_info,
                    ValueIdentifier start_from) {
        unsigned* buf1 = buf.get();
        unsigned* buf2 = buf1 + buf_len_;
        auto const& left_string =
                model::Type::GetValue<std::string>(data_info_left_->GetAt(left_value_id));
        std::size_t const left_size = left_string.size();
        bool dissimilar_found_here = false;
        for (ValueIdentifier value_id_right = start_from; value_id_right != data_right_size_;
             ++value_id_right) {
            auto const& right_string =
                    model::Type::GetValue<std::string>(data_info_right_->GetAt(value_id_right));
            std::size_t const max_dist = std::max(left_size, right_string.size());
            Similarity similarity = 1.0;
            if (max_dist != 0) {
                std::size_t lim = max_dist * (1 - min_sim_);
                // Left has to be second since that's what the function uses to determine the buffer
                // size it needs
                std::size_t dist =
                        LevenshteinDistance(&right_string, &left_string, buf1, buf2, lim, max_dist);
                similarity = (max_dist - dist) / static_cast<Similarity>(max_dist);
                // Don't store 0.0 no matter the value of min_sim.
                if (similarity < min_sim_) similarity = kLowestBound;
            }
            if (similarity == kLowestBound) {
                dissimilar_found_here = true;
                continue;
            }
            AddValue(row_info, value_id_right, similarity);
        }
        return dissimilar_found_here;
    }

    auto Enumerate(bool dissimilar_found) {
        auto additional_bounds = {1.0, kLowestBound};
        std::span additional_results(additional_bounds.begin(), dissimilar_found ? 2 : 1);
        return EncodeResults(std::move(task_data_), additional_results);
    }

    auto GetCalculationMethod() {
        return OneColumnGiven() ? &ValueProcessingWorker::ProcessSame
                                : &ValueProcessingWorker::ProcessFull;
    }

public:
    ValueProcessingWorker(std::shared_ptr<DataInfo const> const& data_info_left,
                          std::shared_ptr<DataInfo const> const& data_info_right,
                          std::vector<indexes::PliCluster> const& clusters_right,
                          Similarity const min_sim)
        : data_info_left_(data_info_left),
          data_info_right_(data_info_right),
          clusters_right_(clusters_right),
          min_sim_(min_sim) {}

    bool OneColumnGiven() const noexcept {
        return data_info_left_ == data_info_right_;
    }

    auto ExecSingleThreaded() {
        Resource resource = AcquireResource();
        auto calculation_method = GetCalculationMethod();
        for (ValueIdentifier left_value_id = 0; left_value_id != data_left_size_; ++left_value_id) {
            (this->*calculation_method)(left_value_id, resource);
        }
        return Enumerate(resource.dissimilar_found);
    }

    auto ExecMultiThreaded(util::WorkerThreadPool& pool) {
        std::atomic<bool> dissimilar_found = false;
        auto calculation_method = GetCalculationMethod();
        auto set_dissimilar = [&dissimilar_found](Resource resource) {
            if (resource.dissimilar_found) dissimilar_found.store(true, std::memory_order::release);
        };
        // Only allocate memory once per thread.
        auto calculate_with_buf = [this, calculation_method](ValueIdentifier left_value_id,
                                                             Resource& resource) {
            (this->*calculation_method)(left_value_id, resource);
        };
        auto acquire_resource = [this]() { return AcquireResource(); };
        pool.ExecIndexWithResource(calculate_with_buf, acquire_resource, data_left_size_,
                                   set_dissimilar);
        return Enumerate(dissimilar_found.load(std::memory_order::acquire));
    }
};
}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {

indexes::SimilarityMeasureOutput LevenshteinSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right) const {
    ValueProcessingWorker worker{data_info_left, data_info_right, clusters_right, min_sim_};
    auto [similarities, enumerated_results] =
            pool_ == nullptr ? worker.ExecSingleThreaded() : worker.ExecMultiThreaded(*pool_);
    // Relying on Levenshtein being symmetrical, only values following the left value were compared.
    // Fill in the other value pairs' results.
    if (worker.OneColumnGiven()) SymmetricClosure(enumerated_results, clusters_right);

    return BuildIndexes(std::move(enumerated_results), std::move(similarities), clusters_right,
                        picker_);
}

LevenshteinSimilarityMeasure::LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim,
                                                           std::size_t size_limit,
                                                           util::WorkerThreadPool* thread_pool)
    : SimilarityMeasure(std::make_unique<model::StringType>(),
                        std::make_unique<model::DoubleType>(), true),
      min_sim_(min_sim),
      size_limit_(size_limit),
      pool_(thread_pool) {}

LevenshteinSimilarityMeasure::Creator::Creator(ColumnIdentifier column1_identifier,
                                               ColumnIdentifier column2_identifier,
                                               model::md::DecisionBoundary min_sim,
                                               std::size_t size_limit)
    : SimilarityMeasureCreator(kName, std::move(column1_identifier), std::move(column2_identifier)),
      min_sim_(min_sim),
      size_limit_(size_limit) {
    if (!(0.0 <= min_sim_ && min_sim_ <= 1.0))
        throw config::ConfigurationError("Minimum similarity out of range");
}

}  // namespace algos::hymd::preprocessing::similarity_measure
