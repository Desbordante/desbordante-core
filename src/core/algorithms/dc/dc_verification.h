#pragma once

#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"
#include "dc.h"
#include "predicate_provider.h"
#include "table/typed_column_data.h"

namespace algos {

class DCVerification final : public Algorithm {
private:
    model::DC dc_;
    std::string dc_string_;
    bool result_;
    config::InputTable input_table_;
    std::vector<model::TypedColumnData> data_;
    std::unique_ptr<ColumnLayoutRelationData> relation_;

    // Verify a DC in general case
    bool VerifyDC();

    // Converts all unequal predicates to a 
    void ConvertToInequality();

    model::DC ParseDCString(std::string dc_string);

    // Check DC for containting only one predicate of form s.A op t.B
    bool CheckOneInequality();

    // Check DC for containting all predicates of form s.C op t.C
    bool CheckAllEquality();

    // Verify DC in case if it contains only one heterogeneous or homogeneous inequality
    // (>, >=, <=, <) predicate and others predicates are only homogeneous equality ones.
    bool VerifyOneInequality();

    // Verify DC in case if it contains all equality predicates
    bool VerifyAllEquality();
    std::vector<std::byte const*> GetTuple(size_t row);
    std::vector<unsigned> ByteVecToUnsignedVec(const std::vector<std::byte const*> vec,
                                               std::vector<unsigned> const& indices);

    void RegisterOptions();
    void MakeExecuteOptsAvailable();

public:
    DCVerification();

    bool DCHolds() {
        return result_;
    };

    void ResetState() final;
    void LoadDataInternal() final;

    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos