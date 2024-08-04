#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "algorithms/md/hymd/preprocessing/build_indexes.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/encode_results.h"

namespace algos::hymd::preprocessing::similarity_measure {

template <typename IndexType>
class ValueProcessingWorker {
protected:
    std::shared_ptr<DataInfo const> const& data_info_left_;
    std::shared_ptr<DataInfo const> const& data_info_right_;
    std::vector<indexes::PliCluster> const& clusters_right_;
    ValidTableResults<Similarity>& task_data_;
    std::size_t const data_left_size_;
    std::size_t const data_right_size_;
    std::atomic<bool> dissimilar_found_ = false;
    IndexType current_index_ = 0;

    void AddValue(RowInfo<Similarity>& row_info, ValueIdentifier value_id, Similarity sim) {
        auto& [sim_value_id_vec, valid_records_number] = row_info;
        sim_value_id_vec.emplace_back(sim, value_id);
        valid_records_number += clusters_right_[value_id].size();
    }

    void Start(auto method) {
        bool found_dissimilar = false;
        ValueIdentifier left_value_id;
        while ((left_value_id = current_index_++) < data_left_size_) {
            bool found_dissimilar_here = (this->*method)(left_value_id);
            if (found_dissimilar_here) found_dissimilar = true;
        }
        current_index_ = data_left_size_;
        if (found_dissimilar) dissimilar_found_.store(true, std::memory_order::release);
    }

    bool ProcessFull(ValueIdentifier const left_value_id) {
        return CalcAndAdd(left_value_id, task_data_[left_value_id], 0);
    }

    bool ProcessSame(ValueIdentifier const left_value_id) {
        RowInfo<Similarity>& row_info = task_data_[left_value_id];
        AddValue(row_info, left_value_id, 1.0);
        return CalcAndAdd(left_value_id, row_info, left_value_id + 1);
    }

    virtual bool CalcAndAdd(ValueIdentifier left_value_id, RowInfo<Similarity>& row_info,
                            ValueIdentifier start_from) = 0;

public:
    ValueProcessingWorker(std::shared_ptr<DataInfo const> const& data_info_left,
                          std::shared_ptr<DataInfo const> const& data_info_right,
                          std::vector<indexes::PliCluster> const& clusters_right,
                          ValidTableResults<Similarity>& task_data)
        : data_info_left_(data_info_left),
          data_info_right_(data_info_right),
          clusters_right_(clusters_right),
          task_data_(task_data),
          data_left_size_(data_info_left->GetElementNumber()),
          data_right_size_(data_info_right->GetElementNumber()) {}

    void StartFull() {
        Start(&ValueProcessingWorker::ProcessFull);
    }

    void StartSame() {
        Start(&ValueProcessingWorker::ProcessSame);
    }

    bool DissimilarFound() const noexcept {
        return dissimilar_found_.load(std::memory_order::acquire);
    }
};

template <typename WorkerType, typename... Args>
auto GetResults(std::shared_ptr<DataInfo const> const& data_info_left,
                std::shared_ptr<DataInfo const> const& data_info_right,
                std::vector<indexes::PliCluster> const& clusters_right, auto start_same,
                auto start_full, auto finish, Args&&... args) {
    ValidTableResults<Similarity> task_data{data_info_left->GetElementNumber()};
    WorkerType worker{data_info_left, data_info_right, clusters_right, task_data,
                      std::forward<Args>(args)...};
    if (data_info_left == data_info_right) {
        start_same(worker);
    } else {
        start_full(worker);
    }
    finish();

    auto additional_bounds = {1.0, kLowestBound};
    std::span additional_results(additional_bounds.begin(), worker.DissimilarFound() ? 2 : 1);
    return EncodeResults(std::move(task_data), additional_results);
}

template <template <typename> class WorkerType, typename... Args>
indexes::SimilarityMeasureOutput MakeIndexesTemplate(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right, util::WorkerThreadPool* pool,
        auto const& picker_, Args&&... args) {
    std::pair<std::vector<preprocessing::Similarity>, EnumeratedValidTableResults> results;

    if (pool == nullptr) {
        results = GetResults<WorkerType<model::Index>>(
                data_info_left, data_info_right, clusters_right,
                [](auto& worker) { worker.StartSame(); }, [](auto& worker) { worker.StartFull(); },
                []() {}, std::forward<Args>(args)...);
    } else {
        results = GetResults<WorkerType<std::atomic<model::Index>>>(
                data_info_left, data_info_right, clusters_right,
                // TODO: account for "bad" measures (not symmetric or equality is not 1.0)
                [pool](auto& worker) { pool->SetWork([&worker]() { worker.StartSame(); }); },
                [pool](auto& worker) { pool->SetWork([&worker]() { worker.StartFull(); }); },
                [pool]() { pool->WorkUntilComplete(); }, std::forward<Args>(args)...);
    }

    auto& [similarities, enumerated_results] = results;
    if (data_info_left == data_info_right) SymmetricClosure(enumerated_results, clusters_right);

    return BuildIndexes(std::move(enumerated_results), std::move(similarities), clusters_right,
                        picker_);
}

}  // namespace algos::hymd::preprocessing::similarity_measure
