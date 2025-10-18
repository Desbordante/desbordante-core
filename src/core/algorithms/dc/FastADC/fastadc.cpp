#include "algorithms/dc/FastADC/fastadc.h"

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <easylogging++.h>

#include "algorithm.h"
#include "builtin.h"
#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "config/option.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/approximate_evidence_inverter.h"
#include "dc/FastADC/util/evidence_aux_structures_builder.h"
#include "dc/FastADC/util/evidence_set_builder.h"
#include "dc/FastADC/util/predicate_builder.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "table/column_index.h"
#include "table/typed_column_data.h"

namespace algos {
namespace fastadc {
class DenialConstraint;
}  // namespace fastadc
}  // namespace algos

namespace algos::dc {

FastADC::FastADC() : Algorithm({}) {
    pred_index_provider_ = std::make_shared<PredicateIndexProvider>();
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void FastADC::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&shard_length_, kShardLength, kDShardLength, 350U});
    RegisterOption(Option{&allow_cross_columns_, kAllowCrossColumns, kDAllowCrossColumns, true});
    RegisterOption(Option{&minimum_shared_value_, kMinimumSharedValue, kDMinimumSharedValue, 0.3});
    RegisterOption(
            Option{&comparable_threshold_, kComparableThreshold, kDComparableThreshold, 0.1});
    RegisterOption(Option{&evidence_threshold_, kEvidenceThreshold, kDEvidenceThreshold, 0.01});
}

void FastADC::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kShardLength, kAllowCrossColumns, kMinimumSharedValue,
                          kComparableThreshold, kEvidenceThreshold});
}

void FastADC::LoadDataInternal() {
    // kMixed type will be treated as a string type
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true, true);

    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: DC mining is meaningless.");
    }
}

void FastADC::SetLimits() {
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

void FastADC::CheckTypes() {
    model::ColumnIndex columns_num = typed_relation_->GetNumColumns();
    unsigned rows_num = typed_relation_->GetNumRows();

    for (model::ColumnIndex column_index = 0; column_index < columns_num; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kMixed) {
            LOG(WARNING) << "Column with index \"" + std::to_string(column_index) +
                                    "\" contains values of different types. Those values will be "
                                    "treated as strings.";
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

void FastADC::PrintResults() {
    LOG(DEBUG) << "Total denial constraints: " << dcs_.TotalDCSize();
    LOG(DEBUG) << "Minimal denial constraints: " << dcs_.MinDCSize();
    LOG(DEBUG) << dcs_.ToString();
}

unsigned long long FastADC::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    LOG(DEBUG) << "Start";

    SetLimits();
    CheckTypes();

    PredicateBuilder predicate_builder(&pred_provider_, pred_index_provider_, allow_cross_columns_,
                                       minimum_shared_value_, comparable_threshold_);
    predicate_builder.BuildPredicateSpace(typed_relation_->GetColumnData());

    PliShardBuilder pli_shard_builder(&int_prov_, &double_prov_, &string_prov_, shard_length_);
    pli_shard_builder.BuildPliShards(typed_relation_->GetColumnData());

    EvidenceAuxStructuresBuilder evidence_aux_structures_builder(predicate_builder);
    evidence_aux_structures_builder.BuildAll();

    EvidenceSetBuilder evidence_set_builder(pli_shard_builder.pli_shards,
                                            evidence_aux_structures_builder.GetPredicatePacks());
    evidence_set_builder.BuildEvidenceSet(evidence_aux_structures_builder.GetCorrectionMap(),
                                          evidence_aux_structures_builder.GetCardinalityMask());

    LOG(DEBUG) << "Built evidence set";
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(DEBUG) << "Current time: " << elapsed_milliseconds.count();

    ApproxEvidenceInverter dcbuilder(predicate_builder, evidence_threshold_,
                                     std::move(evidence_set_builder.evidence_set),
                                     typed_relation_->GetSharedPtrSchema());

    dcs_ = dcbuilder.BuildDenialConstraints();

    PrintResults();

    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(DEBUG) << "Algorithm time: " << elapsed_milliseconds.count();
    return elapsed_milliseconds.count();
}

// TODO: mb make this a list?
std::vector<DenialConstraint> const& FastADC::GetDCs() const {
    return dcs_.GetResult();
}

}  // namespace algos::dc
