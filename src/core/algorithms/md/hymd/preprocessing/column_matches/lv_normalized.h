#pragma once

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/build_indexes.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/pick_lhs_ccv_ids_type.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match_impl.h"
#include "algorithms/md/hymd/preprocessing/column_matches/single_transformer.h"
#include "algorithms/md/hymd/preprocessing/encode_results.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "util/argument_type.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymd::preprocessing::column_matches {
using DistanceFunction = std::function<size_t(std::byte const*, std::byte const*)>;

namespace detail {
template <auto Function, bool MultiThreaded = true>
class LVNormalizedDistanceCalculator {
    using LeftElementType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>;
    using RightElementType = std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>;
    using DistanceType =
            std::invoke_result_t<decltype(Function), LeftElementType, RightElementType>;

    preprocessing::Similarity min_sim_;
    ccv_id_pickers::SimilaritiesPicker picker_;

    class Worker {
        using RowInfoSimilarity = RowInfo<Similarity>;
        std::vector<LeftElementType> const& left_elements_;
        std::vector<RightElementType> const& right_elements_;
        std::vector<indexes::PliCluster> const& right_clusters_;
        preprocessing::Similarity min_sim_;
        std::size_t const num_values_left_ = left_elements_.size();
        std::size_t const num_values_right_ = right_elements_.size();
        ValidTableResults<Similarity> task_data_ =
                util::GetPreallocatedVector<RowInfoSimilarity>(num_values_left_);

        auto Enumerate(bool dissimilar_found) {
            auto additional_bounds = {1.0, kLowestBound};
            std::span additional_results(additional_bounds.begin(), dissimilar_found ? 2 : 1);
            return EncodeResults(std::move(task_data_), additional_results);
        }

        void AddValue(RowInfoSimilarity& row_info, ValueIdentifier value_id_right, Similarity sim) {
            auto& [sim_value_id_vec, valid_records_number] = row_info;
            sim_value_id_vec.emplace_back(sim, value_id_right);
            valid_records_number += right_clusters_[value_id_right].size();
        }

        void CalcFor(RowInfoSimilarity& row_info, ValueIdentifier value_id_left,
                     bool& dissimilar_found) {
            LeftElementType const& left_element = left_elements_[value_id_left];
            DistanceType max_distance = 0;
            std::vector<DistanceType> distances =
                    util::GetPreallocatedVector<DistanceType>(num_values_right_);
            for (RightElementType const& right_element : right_elements_) {
                DistanceType distance = Function(left_element, right_element);
                DESBORDANTE_ASSUME(distance >= 0);
                if (distance > max_distance) max_distance = distance;
                distances.push_back(distance);
            }
            DESBORDANTE_ASSUME(max_distance >= 0);
            if (max_distance == 0) {
                for (ValueIdentifier value_id_right = 0; value_id_right != num_values_right_;
                     ++value_id_right) {
                    AddValue(row_info, value_id_right, 1.0);
                }
            }
            for (ValueIdentifier value_id_right = 0; value_id_right != num_values_right_;
                 ++value_id_right) {
                DistanceType distance = distances[value_id_right];
                preprocessing::Similarity normalized_distance =
                        (max_distance - distance) /
                        static_cast<preprocessing::Similarity>(max_distance);
                if (normalized_distance < min_sim_) {
                    dissimilar_found = true;
                    continue;
                }
                AddValue(row_info, value_id_right, normalized_distance);
            }
        }

    public:
        Worker(std::vector<LeftElementType> const& left_elements,
               std::vector<RightElementType> const& right_elements,
               std::vector<indexes::PliCluster> const& right_clusters,
               preprocessing::Similarity min_sim)
            : left_elements_(left_elements),
              right_elements_(right_elements),
              right_clusters_(right_clusters),
              min_sim_(min_sim) {}

        auto ExecSingleThreaded() {
            bool dissimilar_found = false;
            for (ValueIdentifier left_value_id = 0; left_value_id != num_values_left_;
                 ++left_value_id) {
                CalcFor(task_data_.emplace_back(), left_value_id, dissimilar_found);
            }
            return Enumerate(dissimilar_found);
        }

        auto ExecMultiThreaded(util::WorkerThreadPool& pool) {
            task_data_.assign(num_values_left_, {});
            std::atomic<bool> dissimilar_found = false;
            auto set_dissimilar = [&dissimilar_found](bool dissimilar_found_thread) {
                if (dissimilar_found_thread)
                    dissimilar_found.store(true, std::memory_order::release);
            };
            auto calculate = [this](ValueIdentifier left_value_id, bool& dissimilar_found) {
                CalcFor(task_data_[left_value_id], left_value_id, dissimilar_found);
            };
            auto acquire_resource = []() { return false; };
            pool.ExecIndexWithResource(calculate, acquire_resource, num_values_left_,
                                       set_dissimilar);
            return Enumerate(dissimilar_found.load(std::memory_order::acquire));
        }
    };

public:
    LVNormalizedDistanceCalculator(preprocessing::Similarity min_sim,
                                   ccv_id_pickers::SimilaritiesPicker picker)
        : min_sim_(min_sim), picker_(std::move(picker)) {
        if (!(0.0 <= min_sim && min_sim <= 1.0))
            throw config::ConfigurationError("Minimum similarity out of range");
    }

    indexes::ColumnPairMeasurements Calculate(std::vector<LeftElementType> const* left_elements,
                                              std::vector<RightElementType> const* right_elements,
                                              indexes::KeyedPositionListIndex const& right_pli,
                                              util::WorkerThreadPool* pool_ptr) const {
        std::vector<indexes::PliCluster> const& right_clusters = right_pli.GetClusters();
        Worker worker{*left_elements, *right_elements, right_clusters, min_sim_};
        auto [similarities, enumerated_results] = MultiThreaded && pool_ptr != nullptr
                                                          ? worker.ExecMultiThreaded(*pool_ptr)
                                                          : worker.ExecSingleThreaded();
        return BuildIndexes(std::move(enumerated_results), std::move(similarities), right_clusters,
                            picker_);
    }
};

template <auto Function>
using DistanceBaseTypeTransformer =
        TypeTransformer<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                        std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>>;

template <auto Function, bool... Params>
using DistanceBase = ColumnMatchImpl<DistanceBaseTypeTransformer<Function>,
                                     LVNormalizedDistanceCalculator<Function, Params...>>;
}  // namespace detail

template <auto Function, bool SymmetricAndEq0, bool... Params>
class LVNormalized : public detail::DistanceBase<Function, Params...> {
public:
    using TransformFunctionsOption =
            detail::DistanceBaseTypeTransformer<Function>::TransformFunctionsOption;

    LVNormalized(std::string name, ColumnIdentifier left_column_identifier,
                 ColumnIdentifier right_column_identifier, model::md::DecisionBoundary min_sim,
                 ccv_id_pickers::SimilaritiesPicker picker, TransformFunctionsOption funcs = {})
        : detail::DistanceBase<Function, Params...>(
                  SymmetricAndEq0, std::move(name), std::move(left_column_identifier),
                  std::move(right_column_identifier), {std::move(funcs)},
                  {min_sim, std::move(picker)}) {};

    LVNormalized(std::string name, ColumnIdentifier left_column_identifier,
                 ColumnIdentifier right_column_identifier, model::md::DecisionBoundary min_sim,
                 std::size_t size_limit = 0, TransformFunctionsOption funcs = {})
        : LVNormalized(std::move(name), std::move(left_column_identifier),
                       std::move(right_column_identifier), min_sim,
                       ccv_id_pickers::IndexUniform<Similarity>{size_limit}, std::move(funcs)) {}
};

}  // namespace algos::hymd::preprocessing::column_matches
