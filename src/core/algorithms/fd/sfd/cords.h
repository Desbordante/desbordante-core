#pragma once

#include <list>         // for list
#include <memory>       // for unique_ptr
#include <stddef.h>     // for size_t
#include <string_view>  // for string_view
#include <vector>       // for vector

#include "algorithms/fd/fd_algorithm.h"                     // for FDAlgorithm
#include "config/equal_nulls/type.h"                        // for EqNullsType
#include "config/tabular_data/input_table_type.h"           // for InputTable
#include "correlation.h"                                    // for Correlation
#include "frequency_handler.h"                              // for Frequency...
#include "model/table/column.h"                             // for Column
#include "model/table/column_index.h"                       // for ColumnIndex
#include "model/table/column_layout_typed_relation_data.h"  // for ColumnLay...
#include "primitive_collection.h"                           // for Primitive...

namespace algos {
class Sample;
}

namespace model {
class TypedColumnData;
}

namespace algos {
class Cords : public FDAlgorithm {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    using CorrelationCollection = util::PrimitiveCollection<Correlation>;
    config::InputTable input_table_;
    config::EqNullsType is_null_equal_null_;
    std::unique_ptr<TypedRelation> typed_relation_;

    bool only_sfd_;
    bool fixed_sample_ = false;

    long double minimum_cardinality_;
    long double max_diff_vals_proportion_;
    long double min_sfd_strength_measure_;
    long double min_skew_threshold_;
    long double min_structural_zeroes_proportion_;
    long double max_false_positive_probability_;
    long double delta_;
    size_t max_amount_of_categories_;

    std::vector<bool> is_skewed_;
    std::vector<size_t> domains_;

    CorrelationCollection correlations_collection_;
    FrequencyHandler handler_;
    std::vector<Column> soft_keys_{};
    std::vector<Column> trivial_columns_{};

    void RegisterOptions();
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailableFDInternal() override;
    void ResetStateFd() override;

    unsigned long long ExecuteInternal() override;

    void Init(model::ColumnIndex columns, std::vector<model::TypedColumnData> const &data);

    bool DetectSFD(Sample const &smp);

    // bool DetectAndRegisterSFD(Sample const &smp);

    void SkewHandling(model::ColumnIndex col_i, model::ColumnIndex col_k,
                      std::vector<model::TypedColumnData> const &data, Sample &smp);

    bool IsSoftOrTrivial(model::ColumnIndex col_ind, size_t row_count);

    bool CheckCorrelation(model::ColumnIndex col_i, model::ColumnIndex col_k,
                          std::vector<model::TypedColumnData> const &data, Sample &smp);

    void RegisterCorrelation(model::ColumnIndex lhs_ind, model::ColumnIndex rhs_ind);

public:
    constexpr static std::string_view kFirstPhaseName = "Calculating values frequencies";
    constexpr static std::string_view kSecondPhaseName = "SFD and correlations mining";

    std::vector<Column> const &GetSoftKeys() const noexcept {
        return soft_keys_;
    }

    std::vector<Column> const &GetTrivialColumns() const noexcept {
        return trivial_columns_;
    }

    std::list<Correlation> const &GetCorrelations() const noexcept {
        return correlations_collection_.AsList();
    }

    Cords();
};
}  // namespace algos
