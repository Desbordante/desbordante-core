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

namespace dc {
struct SetComparator {
    bool operator()(std::pair<size_t, size_t> const& lhs,
                    std::pair<size_t, size_t> const& rhs) const {
        return (lhs.first != rhs.first) ? (lhs.first < rhs.first) : (lhs.second < rhs.second);
    }
};

}  // namespace dc

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
    std::set<std::pair<size_t, size_t>, dc::SetComparator> violations_;
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::TypedColumnData> data_;
    config::InputTable input_table_;
    bool do_collect_violations_;
    std::string dc_string_;
    size_t index_offset_;
    bool result_;

    void RegisterOptions();

    void MakeExecuteOptsAvailable();

    bool VerifyOneTuple(dc::DC const& dc);

    bool VerifyTwoTuples(dc::DC const& dc);

    bool VerifyMixed(dc::DC const& dc);

    bool VerifyOneInequality(dc::DC const& dc);

    bool VerifyAllEquality(dc::DC const& dc);

    std::vector<std::byte const*> GetRow(size_t row) const;

    bool Eval(std::vector<std::byte const*> tuple, std::vector<dc::Predicate> preds) const;

    bool ContainsNullOrEmpty(std::vector<Column::IndexType> const& indices, size_t tuple_ind) const;

    std::pair<util::Rect<dc::Point<dc::Component>>, util::Rect<dc::Point<dc::Component>>>
    SearchRanges(std::vector<Column::IndexType> const& all_cols, dc::DC const& ineq_dc,
                 std::vector<std::byte const*> const& tuple) const;

    util::Rect<dc::Point<dc::Component>> SearchMixedRange(dc::DC const& dc, dc::DC const& mixed_dc,
                                                          std::vector<std::byte const*> row);

    std::optional<dc::Point<dc::Component>> ProcessMixed(
            util::KDTree<dc::Point<dc::Component>> const& search_tree,
            std::vector<dc::Predicate> const& const_preds, dc::DC const& two_tuple_dc,
            std::vector<Column::IndexType> const& all_cols, size_t i,
            std::unordered_set<dc::Point<dc::Component>, dc::Point<dc::Component>::Hasher>&
                    res_points);

    dc::Point<dc::Component> MakePoint(std::vector<std::byte const*> const& vec,
                                       std::vector<Column::IndexType> const& indices,
                                       size_t point_ind = 0,
                                       dc::ValType val_type = dc::ValType::kFinite) const;

    dc::DC GetDC(std::vector<dc::Predicate> const& no_diseq_preds,
                 std::vector<dc::Predicate> const& diseq_preds, size_t cur_signs);

    static constexpr frozen::unordered_map<dc::DCType, bool (DCVerifier::*)(dc::DC const&), 5>
            kDCTypeToVerificationMethod{
                    {dc::DCType::kAllEquality, &DCVerifier::VerifyAllEquality},
                    {dc::DCType::kOneInequality, &DCVerifier::VerifyOneInequality},
                    {dc::DCType::kOneTuple, &DCVerifier::VerifyOneTuple},
                    {dc::DCType::kTwoTuples, &DCVerifier::VerifyTwoTuples},
                    {dc::DCType::kMixed, &DCVerifier::VerifyMixed}};

    template <typename InputIt>
    void AddHighlights(InputIt first, InputIt last, size_t cur_index) {
        for (; first != last; ++first) {
            violations_.emplace(first->GetIndex(), cur_index);
        }
    }

public:
    DCVerifier();

    bool Verify(std::string dc_string);

    bool DCHolds() const noexcept {
        return result_;
    }

    std::vector<std::pair<size_t, size_t>> GetViolations() {
        return {violations_.begin(), violations_.end()};
    }

    void ResetState() final {
        violations_.clear();
        result_ = false;
    };

    void LoadDataInternal() final;
    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos
