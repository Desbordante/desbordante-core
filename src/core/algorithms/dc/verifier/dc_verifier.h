#pragma once

#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"
#include "dc/model/dc.h"
#include "dc/model/point.h"
#include "table/typed_column_data.h"
#include "util/kdtree.h"

namespace algos {

class DCVerifier final : public Algorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::TypedColumnData> data_;
    config::InputTable input_table_;
    std::string dc_string_;
    bool result_;

    bool VerifyOneTuple(dc::DC const& dc);

    bool VerifyTwoTuples(dc::DC const& dc);

    bool VerifyMixed(dc::DC const& dc);

    bool VerifyOneInequality(dc::DC const& dc);

    bool VerifyAllEquality(dc::DC const& dc);

    std::vector<dc::Predicate> SplitDC(std::string const& dc_string);

    // Convert all two-tuple equality predicates: s.A == t.B -> (s.A <= t.B and s.A >= t.B)
    dc::DC ConvertEqualities(dc::DC const& dc);

    // Convert all two-tuple disequality predicates:
    // !(φ and s.A != t.B) -> !(φ and s.A < t.B) and !(φ and s.A > t.B)
    // so the DC containing l disequality predicates converts into conjuction of 2^l DC
    std::vector<dc::DC> ConvertDisequalities(dc::DC const& dc);

    bool CheckOneInequality(dc::DC const& dc);

    bool CheckAllEquality(dc::DC const& dc);

    bool CheckOneTuple(dc::DC const& dc);

    bool CheckTwoTuples(dc::DC const& dc);

    dc::DCType GetType(dc::DC const& dc);

    bool Eval(std::vector<std::byte const*> tuple, std::vector<dc::Predicate> preds);

    std::vector<std::byte const*> GetTuple(size_t row);

    uint HashTuple(std::vector<std::byte const*> const& vec, std::vector<uint> const& indices);

    dc::Point<dc::Component> MakePoint(std::vector<std::byte const*> const& vec,
                                       std::vector<uint> const& indices,
                                       dc::ValType val_type = dc::ValType::kFinite);

    std::pair<util::Rect<dc::Point<dc::Component>>, util::Rect<dc::Point<dc::Component>>>
    SearchRanges(dc::DC const& dc, std::vector<std::byte const*> const& tuple);
    bool ContainsNullOrEmpty(std::vector<uint> indices, size_t tuple_ind) const;

    void RegisterOptions();
    void MakeExecuteOptsAvailable();

public:
    DCVerifier();
    bool DCHolds();
    void ResetState() final {};
    void LoadDataInternal() final;
    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos