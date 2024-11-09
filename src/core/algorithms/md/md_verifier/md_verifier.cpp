#include "algorithms/md/md_verifier/md_verifier.h"

#include "config/equal_nulls/option.h"
#include "config/exceptions.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::md {
MDVerifier::MDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void MDVerifier::InitDefaultThresholds() {
    lhs_thresholds_ = std::vector<long double>(lhs_indices_.size(), 1.0);
    rhs_thresholds_ = std::vector<long double>(rhs_indices_.size(), 1.0);
}

void MDVerifier::InitDefaultSimilarityMeasures() {
    for (auto index : lhs_indices_) {
        model::TypedColumnData const& column = relation_->GetColumnData(index);
        model::TypeId type_id = column.GetTypeId();

        if (column.IsNumeric()) {
            lhs_similarity_measures_.push_back(SimilarityMeasure::kEuclidean);
        } else if (type_id == +model::TypeId::kString) {
            lhs_similarity_measures_.push_back(SimilarityMeasure::kLevenshtein);
        } else {
            throw config::ConfigurationError("Column with index \"" + std::to_string(index) +
                                             "\" has upsupported data type.");
        }
    }
    for (auto index : rhs_indices_) {
        model::TypedColumnData const& column = relation_->GetColumnData(index);
        model::TypeId type_id = column.GetTypeId();

        if (column.IsNumeric()) {
            rhs_similarity_measures_.push_back(SimilarityMeasure::kEuclidean);
        } else if (type_id == +model::TypeId::kString) {
            rhs_similarity_measures_.push_back(SimilarityMeasure::kLevenshtein);
        } else {
            throw config::ConfigurationError("Column with index \"" + std::to_string(index) +
                                             "\" has upsupported data type.");
        }
    }
}

void MDVerifier::ValidateIndices(config::IndicesType const& indices,
                                 SimilaritiesType const& similarity_measures) {
    assert(indices.size() == similarity_measures.size());
    for (int i = 0; i < indices.size(); ++i) {
        model::TypedColumnData const& column = relation_->GetColumnData(indices[i]);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kUndefined) {
            throw config::ConfigurationError("Column with index \"" + std::to_string(indices[i]) +
                                             "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw config::ConfigurationError("Column with index \"" + std::to_string(indices[i]) +
                                             "\" contains values of different types.");
        }

        if (similarity_measures[i] == +SimilarityMeasure::kLevenshtein &&
            type_id != +model::TypeId::kString) {
            throw config::ConfigurationError("Similarity measure for column \"" +
                                             std::to_string(indices[i]) +
                                             "\" is only available for string type columns.");
        }

        if (!column.IsNumeric()) {
            throw config::ConfigurationError("Similarity measure for column \"" +
                                             std::to_string(indices[i]) +
                                             "\" is only available for numeric type columns.");
        }
    }
}

void ValidateThresholds(config::IndicesType const& indices, ThresholdsType const& thresholds) {
    assert(indices.size() == thresholds.size());
    for (int i = 0; i < thresholds.size(); ++i) {
        if (thresholds[i] < 0.0 || thresholds[i] > 1.0) {
            throw config::ConfigurationError("Threshold for column with index \"" +
                                             std::to_string(i) + "\" out of range.");
        }
    }
}

void MDVerifier::ResetState() {
    md_holds_ = false;
}

void MDVerifier::LoadDataInternal() {
    relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: MD verifying is meaningless.");
    }
    InitDefault();
}

void MDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_columns = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    auto check_lhs_indices = [this](config::IndicesType const& lhs_indices) {
        ValidateIndices(lhs_indices, lhs_similarity_measures_);
    };

    auto check_rhs_indices = [this](config::IndicesType const& rhs_indices) {
        ValidateIndices(rhs_indices, rhs_similarity_measures_);
    };

    auto check_lhs_thresholds = [this](ThresholdsType const& lhs_thresholds) {
        ValidateThresholds(lhs_indices_, lhs_thresholds_);
    };

    auto check_rhs_thresholds = [this](ThresholdsType const& rhs_thresholds) {
        ValidateThresholds(rhs_indices_, rhs_thresholds_);
    };

    auto check_lhs_similarity_measures = [this](SimilaritiesType const& lhs_similarity_measures) {
        ValidateIndices(lhs_indices_, lhs_similarity_measures);
    };

    auto check_rhs_similarity_measures = [this](SimilaritiesType const& rhs_similarity_measures) {
        ValidateIndices(rhs_indices_, rhs_similarity_measures);
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(Option{&dist_from_null_is_infinity_, kDistFromNullIsInfinity,
                          kDDistFromNullIsInfinity, false});
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_columns, check_lhs_indices));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_columns, check_rhs_indices));
    RegisterOption(Option{&lhs_thresholds_, kMDLhsThresholds, kDMDLhsThresholds}.SetValueCheck(
            check_lhs_thresholds));
    RegisterOption(Option{&rhs_thresholds_, kMDRhsThresholds, kDMDRhsThresholds}.SetValueCheck(
            check_rhs_thresholds));
    RegisterOption(
            Option{&lhs_similarity_measures_, kMDLhsSimilarityMeasures, kDMDLhsSimilarityMeasures}
                    .SetValueCheck(check_lhs_similarity_measures));
    RegisterOption(
            Option{&rhs_similarity_measures_, kMDRhsSimilarityMeasures, kDMDRhsSimilarityMeasures}
                    .SetValueCheck(check_rhs_similarity_measures));
}

void MDVerifier::MakeExecuteOptsAvailable() {}

unsigned long long MDVerifier::ExecuteInternal() {}

}  // namespace algos::md