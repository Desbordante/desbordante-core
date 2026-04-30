#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dc/FastADC/model/denial_constraint.h"
#include "core/algorithms/dc/FastADC/providers/predicate_provider.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/worker_thread_pool.h"

namespace algos::dc {

using namespace fastadc;

class HybridDC : public Algorithm {
private:
    unsigned shard_length_;
    bool allow_cross_columns_;
    double minimum_shared_value_;
    double comparable_threshold_;
    unsigned threads_;

    config::InputTable input_table_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    std::shared_ptr<PredicateIndexProvider> pred_index_provider_;
    PredicateProvider pred_provider_;
    IntIndexProvider int_prov_;
    DoubleIndexProvider double_prov_;
    StringIndexProvider string_prov_;
    std::vector<DenialConstraint> dcs_;

    std::optional<util::WorkerThreadPool> thread_pool_;
    util::WorkerThreadPool* GetThreadPool();

    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;

    void SetLimits();
    void CheckTypes();

    void ResetState() final {
        pred_index_provider_->Clear();
        pred_provider_.Clear();
        int_prov_.Clear();
        double_prov_.Clear();
        string_prov_.Clear();
        dcs_.clear();
    }

    unsigned long long ExecuteInternal() final;

    void RegisterOptions();

public:
    HybridDC();

    std::vector<DenialConstraint> const& GetDCs() const {
        return dcs_;
    }
};

}  // namespace algos::dc
