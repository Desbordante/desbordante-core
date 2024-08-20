#pragma once

#include <memory>
#include <type_traits>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/build_indexes.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/pick_lhs_ccv_ids_type.h"
#include "algorithms/md/hymd/preprocessing/encode_results.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "config/exceptions.h"
#include "util/argument_type.h"
#include "util/get_preallocated_vector.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd::preprocessing::similarity_measure {
template <typename ComparerCreatorSupplier, bool kSymmetric, bool kEqMax,
          bool kMultiThreaded = true>
class BasicCalculator {
    using LeftElementType =
            std::remove_pointer_t<util::ArgumentType<ComparerCreatorSupplier, 0>>::value_type;
    using RightElementType =
            std::remove_pointer_t<util::ArgumentType<ComparerCreatorSupplier, 1>>::value_type;
    // Acquires resources for Comparer based on the data.
    using ComparerCreator =
            std::invoke_result_t<ComparerCreatorSupplier, std::vector<LeftElementType> const*,
                                 std::vector<RightElementType> const*,
                                 indexes::KeyedPositionListIndex const&>;
    // Does the actual comparisons between values, may store resources acquired prior, like a memory
    // buffer
    using Comparer = std::invoke_result_t<ComparerCreator>;

    struct ThreadResource {
        Comparer comparer;
        bool dissimilar_found = false;
    };

    class Worker {
        using RowInfoSimilarity = RowInfo<Similarity>;
        std::vector<LeftElementType> const& left_elements_;
        std::vector<RightElementType> const& right_elements_;
        std::vector<indexes::PliCluster> const& right_clusters_;
        ComparerCreator create_comparer_;
        std::size_t const num_values_left_ = left_elements_.size();
        std::size_t const num_values_right_ = right_elements_.size();
        ValidTableResults<Similarity> task_data_ =
                util::GetPreallocatedVector<RowInfoSimilarity>(num_values_left_);

        void AddValue(RowInfoSimilarity& row_info, ValueIdentifier value_id_right, Similarity sim) {
            auto& [sim_value_id_vec, valid_records_number] = row_info;
            sim_value_id_vec.emplace_back(sim, value_id_right);
            valid_records_number += right_clusters_[value_id_right].size();
        }

        void CalcOnePair(Comparer& comparer, RowInfoSimilarity& row_info,
                         LeftElementType const& left_element, ValueIdentifier value_id_right,
                         bool& dissimilar_found) {
            RightElementType const& right_element = right_elements_[value_id_right];
            Similarity sim = comparer(left_element, right_element);
            if (sim == kLowestBound) {
                dissimilar_found = true;
                return;
            }
            AddValue(row_info, value_id_right, sim);
        }

        void CalcLoop(Comparer& comparer, RowInfoSimilarity& row_info,
                      LeftElementType const& left_element, bool& dissimilar_found,
                      ValueIdentifier from, ValueIdentifier to) {
            for (ValueIdentifier value_id_right = from; value_id_right != to; ++value_id_right) {
                CalcOnePair(comparer, row_info, left_element, value_id_right, dissimilar_found);
            }
        }

        void CalcForFull(Comparer& comparer, ValueIdentifier value_id_left,
                         bool& dissimilar_found) {
            LeftElementType const& left_element = left_elements_[value_id_left];
            RowInfoSimilarity& row_info = task_data_[value_id_left];
            CalcLoop(comparer, row_info, left_element, dissimilar_found, 0, num_values_right_);
        }

        void CalcForSame(Comparer& comparer, ValueIdentifier value_id_left,
                         bool& dissimilar_found) {
            LeftElementType const& left_element = left_elements_[value_id_left];
            RowInfoSimilarity& row_info = task_data_[value_id_left];
            if constexpr (!kSymmetric) {
                CalcLoop(comparer, row_info, left_element, dissimilar_found, 0, value_id_left);
            }
            if constexpr (kEqMax) {
                AddValue(row_info, value_id_left, 1.0);
            } else {
                CalcOnePair(comparer, row_info, left_element, value_id_left, dissimilar_found);
            }
            CalcLoop(comparer, row_info, left_element, dissimilar_found, value_id_left + 1,
                     num_values_right_);
        }

        auto Enumerate(bool dissimilar_found) {
            auto additional_bounds = {1.0, kLowestBound};
            std::span additional_results(additional_bounds.begin(), dissimilar_found ? 2 : 1);
            return EncodeResults(std::move(task_data_), additional_results);
        }

        auto GetCalculationMethod() const noexcept {
            return OneColumnGiven() ? &Worker::CalcForSame : &Worker::CalcForFull;
        }

        ThreadResource AcquireResource() {
            return {create_comparer_()};
        }

    public:
        Worker(std::vector<LeftElementType> const* left_elements,
               std::vector<RightElementType> const* right_elements,
               std::vector<indexes::PliCluster> const& right_clusters,
               ComparerCreator create_comparer)
            : left_elements_(*left_elements),
              right_elements_(*right_elements),
              right_clusters_(right_clusters),
              create_comparer_(std::move(create_comparer)) {}

        bool OneColumnGiven() const noexcept {
            return &left_elements_ == &right_elements_;
        }

        auto ExecSingleThreaded() {
            auto calculation_method = GetCalculationMethod();
            auto [comparer, dissimilar_found] = AcquireResource();
            for (ValueIdentifier left_value_id = 0; left_value_id != num_values_left_;
                 ++left_value_id) {
                task_data_.emplace_back();
                (this->*calculation_method)(comparer, left_value_id, dissimilar_found);
            }
            return Enumerate(dissimilar_found);
        }

        auto ExecMultiThreaded(util::WorkerThreadPool& pool) {
            task_data_.assign(num_values_left_, {});
            std::atomic<bool> dissimilar_found = false;
            auto calculation_method = GetCalculationMethod();
            auto set_dissimilar = [&dissimilar_found](ThreadResource resource) {
                if (resource.dissimilar_found)
                    dissimilar_found.store(true, std::memory_order::release);
            };
            auto calculate = [this, calculation_method](ValueIdentifier left_value_id,
                                                        ThreadResource& resource) {
                (this->*calculation_method)(resource.comparer, left_value_id,
                                            resource.dissimilar_found);
            };
            auto acquire_resource = [this]() { return AcquireResource(); };
            pool.ExecIndexWithResource(calculate, acquire_resource, num_values_left_,
                                       set_dissimilar);
            return Enumerate(dissimilar_found.load(std::memory_order::acquire));
        }
    };

    // May store the comparison function.
    ComparerCreatorSupplier creator_supplier_;
    ccv_id_pickers::SimilaritiesPicker picker_;

public:
    BasicCalculator(ComparerCreatorSupplier creator_supplier,
                    ccv_id_pickers::SimilaritiesPicker picker)
        : creator_supplier_(std::move(creator_supplier)), picker_(std::move(picker)) {}

    indexes::SimilarityMeasureOutput Calculate(std::vector<LeftElementType> const* left_elements,
                                               std::vector<RightElementType> const* right_elements,
                                               indexes::KeyedPositionListIndex const& right_pli,
                                               util::WorkerThreadPool* pool_ptr) const {
        ComparerCreator create_comparer =
                creator_supplier_(left_elements, right_elements, right_pli);
        std::vector<indexes::PliCluster> const& right_clusters = right_pli.GetClusters();
        Worker worker{left_elements, right_elements, right_clusters, std::move(create_comparer)};
        auto [similarities, enumerated_results] = kMultiThreaded && pool_ptr != nullptr
                                                          ? worker.ExecMultiThreaded(*pool_ptr)
                                                          : worker.ExecSingleThreaded();
        if constexpr (kSymmetric) {
            if (worker.OneColumnGiven()) SymmetricClosure(enumerated_results, right_clusters);
        }
        return BuildIndexes(std::move(enumerated_results), std::move(similarities), right_clusters,
                            picker_);
    }
};
}  // namespace algos::hymd::preprocessing::similarity_measure
