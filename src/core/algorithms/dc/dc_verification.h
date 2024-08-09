#pragma once

#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"
#include "dc.h"
#include "kdtree.h"
#include "point.h"
#include "predicate_provider.h"
#include "table/typed_column_data.h"

namespace algos {

class DCVerification final : public Algorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::TypedColumnData> data_;
    config::InputTable input_table_;
    std::string dc_string_;
    model::DC dc_;
    bool result_;

    std::vector<model::Predicate> SplitDC(std::string dc_string);

    // Verify a DC in case if it contains only row homogeneous inequality and eqaulity
    bool VerifyDC();

    // Converts all unequal predicates to a
    void ConvertToInequality();

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
    uint HashTuple(std::vector<std::byte const*> const& vec, std::vector<uint> const& indices);

    Point<Component> MakePoint(std::vector<std::byte const*> const& vec,
                               std::vector<uint> const& indices,
                               ValType val_type = ValType::kFinite);

    kdt::rect<Point<Component>> SearchRange(std::vector<std::byte const*> const& tuple);
    kdt::rect<Point<Component>> InvertRange(kdt::rect<Point<Component>> const& box);

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