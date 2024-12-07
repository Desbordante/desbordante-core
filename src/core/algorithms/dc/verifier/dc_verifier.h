#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <frozen/unordered_map.h>

#include "algorithms/algorithm.h"
#include "algorithms/dc/model/dc.h"
#include "algorithms/dc/model/point.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"
#include "table/typed_column_data.h"
#include "util/kdtree.h"

namespace algos {

class DCVerifier final : public Algorithm {
private:
    // @brief Represents violating tuples of a given table
    // e.g. vector {{1, 4}, {1, 3}} tells us that records with
    // number 1 and 4, 1 and 3 are violating the given Denial Constraint.
    // Thus it is possible to remove first record so the DC holds.
    //
    // If a certain pair contains equal left and right record number
    // it means that that DC is a one-tuple one and it sufficient
    // for a single tuple to violate it. e.g. {{2, 2}}
    std::vector<std::pair<size_t, size_t>> violations_;
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::TypedColumnData> data_;
    config::InputTable input_table_;
    std::string dc_string_;
    size_t index_offset_;
    bool result_;

    void RegisterOptions();

    void MakeExecuteOptsAvailable();

    bool Verify(dc::DC const& dc);

    bool VerifyOneTuple(dc::DC const& dc);

    bool VerifyTwoTuples(dc::DC const& dc);

    bool VerifyMixed(dc::DC const& dc);

    bool VerifyOneInequality(dc::DC const& dc);

    bool VerifyAllEquality(dc::DC const& dc);

    std::vector<dc::Predicate> SplitDC(std::string const& dc_string);

    // Convert all two-tuple equality predicates: s.A == t.B -> (s.A <= t.B and s.A >= t.B)
    dc::DC ConvertEqualities(dc::DC const& dc);

    std::vector<std::byte const*> GetRow(size_t row);

    bool Eval(std::vector<std::byte const*> tuple, std::vector<dc::Predicate> preds);

    bool ContainsNullOrEmpty(std::vector<Column::IndexType> const& indices, size_t tuple_ind) const;

    std::pair<util::Rect<dc::Point<dc::Component>>, util::Rect<dc::Point<dc::Component>>>
    SearchRanges(dc::DC const& dc, std::vector<std::byte const*> const& tuple);

    void ProcessMixed(std::vector<dc::Predicate> const& preds,
                      util::KDTree<dc::Point<dc::Component>>& insert_tree,
                      util::KDTree<dc::Point<dc::Component>> const& search_tree, dc::DC const& dc,
                      size_t i, std::vector<Column::IndexType> const& cols_indices, bool& res);

    void AddHighlights(std::vector<dc::Point<dc::Component>> const& vec, size_t index);

    dc::Point<dc::Component> MakePoint(std::vector<std::byte const*> const& vec,
                                       std::vector<Column::IndexType> const& indices,
                                       size_t point_ind = 0,
                                       dc::ValType val_type = dc::ValType::kFinite);

    dc::DC GetDC(std::vector<dc::Predicate> const& no_diseq_preds,
                 std::vector<dc::Predicate> const& diseq_preds, size_t cur_signs);

    static constexpr frozen::unordered_map<dc::DCType, bool (DCVerifier::*)(dc::DC const&), 5>
            kDCTypeToVerificationMethod{
                    {dc::DCType::kAllEquality, &DCVerifier::VerifyAllEquality},
                    {dc::DCType::kOneInequality, &DCVerifier::VerifyOneInequality},
                    {dc::DCType::kOneTuple, &DCVerifier::VerifyOneTuple},
                    {dc::DCType::kTwoTuples, &DCVerifier::VerifyTwoTuples},
                    {dc::DCType::kMixed, &DCVerifier::VerifyMixed}};

public:
    DCVerifier();

    bool DCHolds() const noexcept {
        return result_;
    }

    auto const& GetViolations() {
        return violations_;
    }

    void ResetState() final {};
    void LoadDataInternal() final;
    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos
