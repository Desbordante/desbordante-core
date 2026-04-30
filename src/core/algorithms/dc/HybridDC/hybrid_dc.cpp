#include "core/algorithms/dc/HybridDC/hybrid_dc.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"
#include "core/algorithms/dc/FastADC/util/evidence_set_builder.h"
#include "core/algorithms/dc/FastADC/util/predicate_builder.h"
#include "core/algorithms/dc/HybridDC/hei_inverter.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/logger.h"

namespace algos::dc {

HybridDC::HybridDC() : Algorithm() {
    pred_index_provider_ = std::make_shared<PredicateIndexProvider>();
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void HybridDC::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&shard_length_, kShardLength, kDShardLength, 350U});
    RegisterOption(Option{&allow_cross_columns_, kAllowCrossColumns, kDAllowCrossColumns, true});
    RegisterOption(Option{&minimum_shared_value_, kMinimumSharedValue, kDMinimumSharedValue, 0.3});
    RegisterOption(
            Option{&comparable_threshold_, kComparableThreshold, kDComparableThreshold, 0.1});
    RegisterOption(Option{&threads_, kThreads, kDThreads, 1U});
}

void HybridDC::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kShardLength, kAllowCrossColumns, kMinimumSharedValue,
                          kComparableThreshold, kThreads});
}

util::WorkerThreadPool* HybridDC::GetThreadPool() {
    if (threads_ <= 1) {
        thread_pool_.reset();
        return nullptr;
    }

    if (!thread_pool_ || thread_pool_->ThreadNum() != threads_) {
        thread_pool_.emplace(threads_);
    }
    return &*thread_pool_;
}

void HybridDC::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true, true);

    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: DC mining is meaningless.");
    }
}

void HybridDC::SetLimits() {
    unsigned all_rows_num = typed_relation_->GetNumRows();

    if (shard_length_ > all_rows_num) {
        throw std::invalid_argument(
                "'shard_length' (" + std::to_string(shard_length_) +
                ") must be less or equal to the number of rows in the table (total "
                "rows: " +
                std::to_string(all_rows_num) + ")");
    }
    if (shard_length_ == 0) shard_length_ = all_rows_num;
}

void HybridDC::CheckTypes() {
    model::ColumnIndex columns_num = typed_relation_->GetNumColumns();
    unsigned rows_num = typed_relation_->GetNumRows();

    for (model::ColumnIndex column_index = 0; column_index < columns_num; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kMixed) {
            LOG_WARN(
                    "Column with index \"{}\" contains values of different types. Those values "
                    "will be "
                    "treated as strings.",
                    column_index);
        } else if (!column.IsNumeric() && type_id != +model::TypeId::kString) {
            throw std::invalid_argument(
                    "Column with index \"" + std::to_string(column_index) +
                    "\" is of unsupported type. Only numeric and string types are supported.");
        }

        for (std::size_t row_index = 0; row_index < rows_num; ++row_index) {
            if (column.IsNullOrEmpty(row_index)) {
                throw std::runtime_error("Some of the value coordinates are null or empty.");
            }
        }
    }
}

unsigned long long HybridDC::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    LOG_DEBUG("Start");

    SetLimits();
    CheckTypes();

    PredicateBuilder predicate_builder(&pred_provider_, pred_index_provider_, allow_cross_columns_,
                                       minimum_shared_value_, comparable_threshold_);
    predicate_builder.BuildPredicateSpace(typed_relation_->GetColumnData());

    PliShardBuilder pli_shard_builder(&int_prov_, &double_prov_, &string_prov_, shard_length_);
    pli_shard_builder.BuildPliShards(typed_relation_->GetColumnData());

    EvidenceAuxStructuresBuilder evidence_aux_structures_builder(predicate_builder);
    evidence_aux_structures_builder.BuildAll();

    util::WorkerThreadPool* thread_pool = GetThreadPool();
    EvidenceSetBuilder evidence_set_builder(
            pli_shard_builder.pli_shards, evidence_aux_structures_builder.GetPredicatePacks(),
            evidence_aux_structures_builder.GetNumberOfBitsInClue(), thread_pool);
    evidence_set_builder.BuildEvidenceSet(evidence_aux_structures_builder.GetCorrectionMap(),
                                          evidence_aux_structures_builder.GetCardinalityMask());

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Built evidence set, time: {}", elapsed_milliseconds.count());

    if (const char* dump = std::getenv("PREDICATE_DUMP")) {
        FILE* f = fopen(dump, "w");
        if (f) {
            auto const& objects = pred_index_provider_->GetObjects();
            for (size_t i = 0; i < objects.size(); ++i) {
                fprintf(f, "%zu\t%s\n", i, objects[i]->ToString().c_str());
            }
            fclose(f);
        }
    }

    if (const char* dump = std::getenv("EVIDENCE_DUMP")) {
        FILE* f = fopen(dump, "w");
        if (f) {
            for (auto const& ev : evidence_set_builder.evidence_set) {
                bool first = true;
                for (size_t i = ev.evidence._Find_first(); i < ev.evidence.size();
                     i = ev.evidence._Find_next(i)) {
                    if (!first) fprintf(f, " ");
                    fprintf(f, "%zu", i);
                    first = false;
                }
                fprintf(f, "\n");
            }
            fclose(f);
        }
    }

    size_t n_predicates = predicate_builder.PredicateCount();
    auto mutex_map = predicate_builder.TakeMutexMap();

    HEIInverter hei_inverter(n_predicates, mutex_map, pred_index_provider_,
                              typed_relation_->GetSharedPtrSchema(), threads_);
    dcs_ = hei_inverter.Run(evidence_set_builder.evidence_set);

    LOG_DEBUG("Total minimal DCs found: {}", dcs_.size());

    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Algorithm time: {}", elapsed_milliseconds.count());
    return elapsed_milliseconds.count();
}

}  // namespace algos::dc
