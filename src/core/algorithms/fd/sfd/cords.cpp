#include "core/algorithms/fd/sfd/cords.h"

#include <chrono>
#include <utility>
#include <vector>

#include "core/algorithms/fd/sfd/contingency_table.h"
#include "core/algorithms/fd/sfd/frequency_handler.h"
#include "core/algorithms/fd/sfd/sample.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/column_index.h"
#include "core/model/table/typed_column_data.h"

namespace algos {
Cords::Cords() : FDAlgorithm({kFirstPhaseName, kSecondPhaseName}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void Cords::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_param = [](long double param) {
        if (param <= 0 || param >= 1) throw config::ConfigurationError("Parameter out of range");
    };
    auto check_max_false_positive = [](long double param) {
        if (param <= 0 || param >= 0.39)
            throw config::ConfigurationError(
                    "Maximum probability of a false-positive result is out of range");
    };
    auto check_positive = [](size_t param) {
        if (param <= 0) throw config::ConfigurationError("Parameter out of range");
    };
    auto check_delta = [this](long double param) {
        if (param <= 0 || param >= 1) throw config::ConfigurationError("delta out of range");
        if (param < minimum_cardinality_) {
            throw config::ConfigurationError("delta must be greater than minimum_cardinality_");
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));

    RegisterOption(Option{&only_sfd_, kOnlySFD, kDOnlySFD});
    RegisterOption(Option{&minimum_cardinality_, kMinCard, kDMinCard}.SetValueCheck(check_param));
    RegisterOption(
            Option{&max_diff_vals_proportion_, kMaxDiffValsProportion, kDMaxDiffValsProportion}
                    .SetValueCheck(check_param));
    RegisterOption(
            Option{&min_sfd_strength_measure_, kMinSFDStrengthMeasure, kDMinSFDStrengthMeasure}
                    .SetValueCheck(check_param));
    RegisterOption(
            Option{&min_skew_threshold_, kMinSkewThreshold, kDMinSkewThreshold}.SetValueCheck(
                    check_param));
    RegisterOption(Option{&min_structural_zeroes_proportion_, kMinStructuralZeroesAmount,
                          kDMinStructuralZeroesAmount}
                           .SetValueCheck(check_param));
    RegisterOption(Option{&max_false_positive_probability_, kMaxFalsePositiveProbability,
                          kDMaxFalsePositiveProbability}
                           .SetValueCheck(check_max_false_positive));

    RegisterOption(Option{&delta_, kDelta, kDDelta}.SetValueCheck(check_delta));
    RegisterOption(
            Option{&max_amount_of_categories_, kMaxAmountOfCategories, kDMaxAmountOfCategories}
                    .SetValueCheck(check_positive));

    RegisterOption(Option{&fixed_sample_, kFixedSample, kDFixedSample, false});
}

void Cords::MakeExecuteOptsAvailableFDInternal() {
    using namespace config::names;
    MakeOptionsAvailable({kOnlySFD, kMinCard, kMaxDiffValsProportion, kMinSFDStrengthMeasure,
                          kMinSkewThreshold, kMinStructuralZeroesAmount,
                          kMaxFalsePositiveProbability, kDelta, kMaxAmountOfCategories,
                          kFixedSample});
}

void Cords::ResetStateFd() {
    is_skewed_.clear();
    domains_.clear();
    soft_keys_.clear();
    trivial_columns_.clear();
    correlations_collection_.Clear();
}

void Cords::LoadDataInternal() {
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
}

bool Cords::DetectSFD(Sample const &smp) {
    return smp.GetConcatCardinality() <= max_diff_vals_proportion_ * smp.GetRowIndices().size() &&
           (smp.GetLhsCardinality() >=
            (1 - min_sfd_strength_measure_) * smp.GetConcatCardinality());
}

void Cords::SkewHandling(model::ColumnIndex col_i, model::ColumnIndex col_k,
                         std::vector<model::TypedColumnData> const &data, Sample &smp) {
    for (model::ColumnIndex col_ind : {col_i, col_k}) {
        if (handler_.GetColumnFrequencySum(col_ind) >=
            (1 - min_skew_threshold_) * data[col_ind].GetNumRows()) {
            is_skewed_[col_ind] = true;
            domains_[col_ind] = handler_.ColumnFrequencyMapSize(col_ind);
            smp.Filter(handler_, data, col_ind);
        } else {
            domains_[col_ind] =
                    std::min(handler_.GetColumnCardinality(col_ind), max_amount_of_categories_);
        }
    }
}

void Cords::Init(model::ColumnIndex columns, std::vector<model::TypedColumnData> const &data) {
    is_skewed_.resize(columns, false);
    domains_.resize(columns, 0);
    handler_.InitFrequencyHandler(data, columns, max_amount_of_categories_);
}

void Cords::RegisterCorrelation(model::ColumnIndex lhs_ind, model::ColumnIndex rhs_ind) {
    Column lhs_col(typed_relation_->GetSchema(),
                   typed_relation_->GetSchema()->GetColumn(lhs_ind)->GetName(), lhs_ind);
    Column rhs_col(typed_relation_->GetSchema(),
                   typed_relation_->GetSchema()->GetColumn(rhs_ind)->GetName(), rhs_ind);
    Correlation correlation_to_register(lhs_col, rhs_col);
    correlations_collection_.Register(std::move(correlation_to_register));
}

bool Cords::CheckCorrelation(model::ColumnIndex col_i, model::ColumnIndex col_k,
                             std::vector<model::TypedColumnData> const &data, Sample &smp) {
    SkewHandling(col_i, col_k, data, smp);

    ContingencyTable cont_table(col_i, col_k, domains_);
    cont_table.FillTable(smp, data, handler_, is_skewed_, domains_);

    return cont_table.TooMuchStructuralZeroes(domains_, min_structural_zeroes_proportion_) ||
           cont_table.ChiSquaredTest(smp, domains_, max_false_positive_probability_);
}

bool Cords::IsSoftOrTrivial(model::ColumnIndex col_ind, size_t row_count) {
    if (handler_.GetColumnCardinality(col_ind) >= (1 - minimum_cardinality_) * row_count) {
        Column target(typed_relation_->GetSchema(),
                      typed_relation_->GetSchema()->GetColumn(col_ind)->GetName(), col_ind);
        soft_keys_.push_back(std::move(target));
        return true;
    }
    if (handler_.GetColumnCardinality(col_ind) == 1) {
        Column target(typed_relation_->GetSchema(),
                      typed_relation_->GetSchema()->GetColumn(col_ind)->GetName(), col_ind);
        trivial_columns_.push_back(target);
        return true;
    }
    return false;
}

unsigned long long Cords::ExecuteInternal() {
    std::vector<model::TypedColumnData> const &data = typed_relation_->GetColumnData();

    size_t row_count = data.front().GetNumRows();
    model::ColumnIndex column_count = data.size();

    Init(column_count, data);

    auto start_time = std::chrono::system_clock::now();

    SetProgress(kTotalProgressPercent);
    ToNextProgressPhase();

    std::vector<bool> is_soft_or_trivial(column_count);
    for (model::ColumnIndex col_ind = 0; col_ind != column_count; ++col_ind)
        is_soft_or_trivial[col_ind] = IsSoftOrTrivial(col_ind, row_count);

    auto sort_indices_by_cardinality =
            [this](model::ColumnIndex ind1,
                   model::ColumnIndex ind2) -> std::pair<model::ColumnIndex, model::ColumnIndex> {
        if (handler_.GetColumnCardinality(ind2) > handler_.GetColumnCardinality(ind1)) {
            return {ind2, ind1};
        }
        return {ind1, ind2};
    };

    for (model::ColumnIndex ind1 = 0; ind1 < column_count - 1; ind1++) {
        if (is_soft_or_trivial[ind1]) continue;

        for (model::ColumnIndex ind2 = ind1 + 1; ind2 < column_count; ind2++) {
            if (is_soft_or_trivial[ind2]) continue;

            auto [col_i, col_k] = sort_indices_by_cardinality(ind1, ind2);

            unsigned long long sample_size = Sample::CalculateSampleSize(
                    handler_.GetColumnCardinality(col_i), handler_.GetColumnCardinality(col_k),
                    max_false_positive_probability_, delta_);

            Sample smp(fixed_sample_, sample_size, row_count, col_i, col_k, data,
                       typed_relation_->GetSchema());

            bool sfd_detected = DetectSFD(smp);

            if (sfd_detected) {
                RegisterFd(smp.GetLhsVertical(), smp.GetRhsColumn(),
                           typed_relation_->GetSharedPtrSchema());
            }

            if (sfd_detected || only_sfd_) {
                continue;
            }

            if (CheckCorrelation(col_i, col_k, data, smp)) {
                RegisterCorrelation(col_i, col_k);
            }
        }
    }

    SetProgress(kTotalProgressPercent);
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_time.count();
}
}  // namespace algos
